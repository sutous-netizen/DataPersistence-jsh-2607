#include <cstdio>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Json.h"

using ::testing::HasSubstr;
using ::testing::InSequence;

namespace {

    // A minimal write-sink abstraction used only by these tests, to demonstrate
    // verifying interactions with a consumer of serialized JSON via gmock.
    class JsonSink {
    public:
        virtual ~JsonSink() = default;
        virtual void Write(const std::string& serialized) = 0;
    };

    class MockJsonSink : public JsonSink {
    public:
        MOCK_METHOD(void, Write, (const std::string& serialized), (override));
    };

} // namespace

TEST(JsonParseTest, ParsesObjectWithVariousTypes) {
    json::Value v = json::Value::Parse(
        R"({"str":"hello","num":42,"pi":3.14,"flag":true,"nothing":null,"list":[1,2,3]})");

    ASSERT_TRUE(v.IsObject());
    EXPECT_EQ(v["str"].AsString(), "hello");
    EXPECT_DOUBLE_EQ(v["num"].AsNumber(), 42.0);
    EXPECT_DOUBLE_EQ(v["pi"].AsNumber(), 3.14);
    EXPECT_TRUE(v["flag"].AsBool());
    EXPECT_TRUE(v["nothing"].IsNull());
    ASSERT_TRUE(v["list"].IsArray());
    EXPECT_EQ(v["list"].AsArray().size(), 3u);
}

TEST(JsonParseTest, ParsesNestedArraysAndObjects) {
    json::Value v = json::Value::Parse(R"({"outer":{"inner":[1,{"deep":true}]}})");

    ASSERT_TRUE(v["outer"].IsObject());
    ASSERT_TRUE(v["outer"]["inner"].IsArray());
    EXPECT_DOUBLE_EQ(v["outer"]["inner"][0].AsNumber(), 1.0);
    EXPECT_TRUE(v["outer"]["inner"][1]["deep"].AsBool());
}

TEST(JsonParseTest, ParsesEscapedAndUnicodeStrings) {
    json::Value v = json::Value::Parse(R"("line1\nline2\tAé")");

    EXPECT_THAT(v.AsString(), HasSubstr("\n"));
    EXPECT_THAT(v.AsString(), HasSubstr("\t"));
    EXPECT_THAT(v.AsString(), HasSubstr("A"));
}

TEST(JsonParseTest, ParsesNumberFormats) {
    EXPECT_DOUBLE_EQ(json::Value::Parse("-12").AsNumber(), -12.0);
    EXPECT_DOUBLE_EQ(json::Value::Parse("0.5").AsNumber(), 0.5);
    EXPECT_DOUBLE_EQ(json::Value::Parse("1e3").AsNumber(), 1000.0);
    EXPECT_DOUBLE_EQ(json::Value::Parse("-2.5E-2").AsNumber(), -0.025);
}

TEST(JsonParseErrorTest, ThrowsOnMalformedInput) {
    EXPECT_THROW(json::Value::Parse("{"), json::ParseException);
    EXPECT_THROW(json::Value::Parse("[1,]"), json::ParseException);
    EXPECT_THROW(json::Value::Parse(R"({"a":1)"), json::ParseException);
    EXPECT_THROW(json::Value::Parse("nul"), json::ParseException);
    EXPECT_THROW(json::Value::Parse(R"("unterminated)"), json::ParseException);
}

TEST(JsonParseErrorTest, ReportsLineAndColumnOfFailure) {
    try {
        json::Value::Parse("{\n  \"a\": ,\n}");
        FAIL() << "Expected json::ParseException to be thrown";
    } catch (const json::ParseException& ex) {
        EXPECT_EQ(ex.Line(), 2u);
        EXPECT_GT(ex.Column(), 0u);
    }
}

TEST(JsonAccessTest, ObjectOperatorAutoVivifiesMembers) {
    json::Value v;
    v["key"] = json::Value("value");

    EXPECT_TRUE(v.IsObject());
    EXPECT_EQ(v["key"].AsString(), "value");
    EXPECT_TRUE(v.Contains("key"));
    EXPECT_FALSE(v.Contains("missing"));
}

TEST(JsonAccessTest, ConstOperatorThrowsWhenMemberMissing) {
    const json::Value v = json::Value::MakeObject();
    EXPECT_THROW(v["missing"], std::runtime_error);
}

TEST(JsonAccessTest, TypeMismatchAccessorsThrow) {
    json::Value v("text");
    EXPECT_THROW(v.AsNumber(), std::runtime_error);
    EXPECT_THROW(v.AsBool(), std::runtime_error);
    EXPECT_THROW(v.AsArray(), std::runtime_error);
    EXPECT_THROW(v.AsObject(), std::runtime_error);
}

TEST(JsonAccessTest, ArrayPushBackAndIndexedAccess) {
    json::Value arr = json::Value::MakeArray();
    arr.PushBack(1);
    arr.PushBack("two");
    arr.PushBack(true);

    ASSERT_EQ(arr.AsArray().size(), 3u);
    EXPECT_DOUBLE_EQ(arr[0].AsNumber(), 1.0);
    EXPECT_EQ(arr[1].AsString(), "two");
    EXPECT_TRUE(arr[2].AsBool());
}

TEST(JsonSerializeTest, CompactRoundTripPreservesData) {
    json::Value v = json::Value::Parse(R"({"a":1,"b":[true,false,null]})");
    json::Value reparsed = json::Value::Parse(v.ToString());

    EXPECT_EQ(reparsed.ToString(), v.ToString());
}

TEST(JsonSerializeTest, PrettyPrintUsesIndentAndNewlines) {
    json::Value v = json::Value::MakeObject();
    v.Set("a", 1);

    std::string pretty = v.ToString(2);

    EXPECT_THAT(pretty, HasSubstr("\n"));
    EXPECT_THAT(pretty, HasSubstr("  \"a\""));
}

TEST(JsonSerializeTest, EmptyArrayAndObjectSerializeCompactly) {
    EXPECT_EQ(json::Value::MakeArray().ToString(2), "[]");
    EXPECT_EQ(json::Value::MakeObject().ToString(2), "{}");
}

TEST(JsonFileIoTest, SaveAndLoadRoundTrip) {
    json::Value v = json::Value::MakeObject();
    v.Set("name", "crud");
    v.Set("values", json::Value::MakeArray());
    v["values"].PushBack(1);
    v["values"].PushBack(2);

    std::string path = ::testing::TempDir() + "json_test_roundtrip.json";
    v.SaveToFile(path, 2);

    json::Value loaded = json::Value::ParseFile(path);
    EXPECT_EQ(loaded["name"].AsString(), "crud");
    ASSERT_EQ(loaded["values"].AsArray().size(), 2u);
    EXPECT_DOUBLE_EQ(loaded["values"][0].AsNumber(), 1.0);

    std::remove(path.c_str());
}

TEST(JsonFileIoTest, ParseFileThrowsWhenFileMissing) {
    EXPECT_THROW(
        json::Value::ParseFile(::testing::TempDir() + "definitely_missing_file.json"),
        std::runtime_error);
}

// Demonstrates verifying a consumer's interaction with serialized JSON using
// gmock's MOCK_METHOD/EXPECT_CALL rather than plain assertions on the string.
TEST(JsonMockIntegrationTest, SinkReceivesExactSerializedPayload) {
    json::Value v = json::Value::MakeObject();
    v.Set("status", "ok");

    MockJsonSink sink;
    EXPECT_CALL(sink, Write(v.ToString())).Times(1);

    sink.Write(v.ToString());
}

TEST(JsonMockIntegrationTest, SinkReceivesEachBatchItemInOrder) {
    std::vector<json::Value> batch;
    batch.push_back(json::Value::Parse(R"({"id":1})"));
    batch.push_back(json::Value::Parse(R"({"id":2})"));

    MockJsonSink sink;
    {
        InSequence seq;
        for (const auto& item : batch) {
            EXPECT_CALL(sink, Write(item.ToString())).Times(1);
        }
    }

    for (const auto& item : batch) {
        sink.Write(item.ToString());
    }
}
