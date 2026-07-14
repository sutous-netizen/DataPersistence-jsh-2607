#include <cstdio>
#include <sstream>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "ConsoleApp.h"
#include "JsonRecordStore.h"

using ::testing::HasSubstr;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;

namespace {

    std::string TempStorePath(const std::string& name) {
        return ::testing::TempDir() + name;
    }

    json::Value MakeRecord(int id, const std::string& name) {
        json::Value record = json::Value::MakeObject();
        record.Set("id", id);
        record.Set("name", name);
        return record;
    }

    // A mock IRecordStore, used to verify ConsoleApp's interaction with its
    // storage dependency (call counts/arguments) independent of real file I/O.
    class MockRecordStore : public crudapp::IRecordStore {
    public:
        MOCK_METHOD(int, Create, (json::Value fields), (override));
        MOCK_METHOD(const json::Value*, Find, (int id), (const, override));
        MOCK_METHOD(json::Value::Array, FindByField, (const std::string& key, const std::string& value), (const, override));
        MOCK_METHOD(const json::Value::Array&, All, (), (const, override));
        MOCK_METHOD(bool, Update, (int id, const json::Value& fields), (override));
        MOCK_METHOD(bool, Delete, (int id), (override));
    };

} // namespace

TEST(ConsoleAppTest, CreateThenReadAllShowsTheNewRecord) {
    std::string path = TempStorePath("console_app_create.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in(
        "1\n"          // Create
        "name=Alice\n"
        "role=engineer\n"
        "\n"           // blank line ends field input
        "2\n"          // Read all
        "6\n");        // Exit
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Created record with id 1"));
    EXPECT_THAT(out.str(), HasSubstr("\"name\": \"Alice\""));
    EXPECT_THAT(out.str(), HasSubstr("\"role\": \"engineer\""));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, ReadOneReportsMissingRecord) {
    std::string path = TempStorePath("console_app_read_one_missing.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in(
        "3\n"     // Read one
        "42\n"    // id
        "6\n");   // Exit
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("No record with id 42"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, UpdateChangesExistingRecord) {
    std::string path = TempStorePath("console_app_update.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);
    int id = store.Create([] {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", "Alice");
        fields.Set("role", "engineer");
        return fields;
    }());

    std::istringstream in(
        "4\n" + std::to_string(id) + "\n"
        "role=manager\n"
        "\n"
        "6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Updated record with id " + std::to_string(id)));
    ASSERT_NE(store.Find(id), nullptr);
    EXPECT_EQ((*store.Find(id))["role"].AsString(), "manager");

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, DeleteRemovesRecord) {
    std::string path = TempStorePath("console_app_delete.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);
    int id = store.Create([] {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", "Alice");
        return fields;
    }());

    std::istringstream in("5\n" + std::to_string(id) + "\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Deleted record with id " + std::to_string(id)));
    EXPECT_EQ(store.Find(id), nullptr);

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, ReadAllListsRecordsSortedById) {
    std::string path = TempStorePath("console_app_read_all_sorted.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);
    store.Create([] {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", "Bob");
        return fields;
    }());
    store.Create([] {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", "Alice");
        return fields;
    }());

    std::istringstream in("2\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    std::string text = out.str();
    EXPECT_LT(text.find("Bob"), text.find("Alice"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, UnknownOptionReportsError) {
    std::string path = TempStorePath("console_app_unknown_option.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("9\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Unknown option: 9"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, ReadOneWithNonNumericIdReportsMissingRecord) {
    std::string path = TempStorePath("console_app_non_numeric_id.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("3\nabc\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("No record with id -1"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, CreateIgnoresInvalidKeyValueLineButKeepsValidOnes) {
    std::string path = TempStorePath("console_app_invalid_field_line.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("1\nname=Alice\nnotakeyvalue\nrole=engineer\n\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Ignoring invalid line (expected key=value): notakeyvalue"));
    EXPECT_THAT(out.str(), HasSubstr("Created record with id 1"));
    ASSERT_NE(store.Find(1), nullptr);
    EXPECT_EQ((*store.Find(1))["name"].AsString(), "Alice");
    EXPECT_EQ((*store.Find(1))["role"].AsString(), "engineer");

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, CreateWithNoFieldsStillAssignsAnId) {
    std::string path = TempStorePath("console_app_empty_fields.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("1\n\n6\n"); // blank line immediately ends field input
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Created record with id 1"));
    ASSERT_NE(store.Find(1), nullptr);
}

TEST(ConsoleAppTest, CreateCallsStoreCreateWithParsedFields) {
    MockRecordStore store;
    EXPECT_CALL(store, Create(_)).WillOnce(Invoke([](json::Value fields) {
        EXPECT_EQ(fields["name"].AsString(), "Alice");
        return 7;
    }));

    std::istringstream in("1\nname=Alice\n\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Created record with id 7"));
}

TEST(ConsoleAppTest, DeleteCallsStoreDeleteWithParsedId) {
    MockRecordStore store;
    EXPECT_CALL(store, Delete(5)).WillOnce(Return(true));

    std::istringstream in("5\n5\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("Deleted record with id 5"));
}

TEST(ConsoleAppTest, ReadOneWithKeyValueSearchesByField) {
    std::string path = TempStorePath("console_app_search_by_field.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);
    store.Create([] {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", "Alice");
        fields.Set("role", "engineer");
        return fields;
    }());
    store.Create([] {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", "Bob");
        fields.Set("role", "designer");
        return fields;
    }());

    std::istringstream in("3\nrole=designer\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("\"name\": \"Bob\""));
    EXPECT_THAT(out.str(), ::testing::Not(HasSubstr("\"name\": \"Alice\"")));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, ReadOneWithKeyValueReportsNoMatches) {
    std::string path = TempStorePath("console_app_search_by_field_missing.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("3\nrole=manager\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("No records with role=manager"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, UpdateOfMissingIdReportsNoRecord) {
    std::string path = TempStorePath("console_app_update_missing.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("4\n99\nrole=manager\n\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("No record with id 99"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, DeleteOfMissingIdReportsNoRecord) {
    std::string path = TempStorePath("console_app_delete_missing.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    std::istringstream in("5\n99\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    EXPECT_THAT(out.str(), HasSubstr("No record with id 99"));

    std::remove(path.c_str());
}

TEST(ConsoleAppTest, ReadAllCallsStoreAllOnceAndSortsResultsByRecordId) {
    MockRecordStore store;
    json::Value::Array unsorted;
    unsorted.push_back(MakeRecord(2, "Bob"));
    unsorted.push_back(MakeRecord(1, "Alice"));

    EXPECT_CALL(store, All()).Times(1).WillOnce(ReturnRef(unsorted));

    std::istringstream in("2\n6\n");
    std::ostringstream out;

    crudapp::ConsoleApp app(store, in, out);
    app.Run();

    std::string text = out.str();
    EXPECT_LT(text.find("Alice"), text.find("Bob"));
}
