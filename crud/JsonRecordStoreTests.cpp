#include <cstdio>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "JsonRecordStore.h"

namespace {

    std::string TempStorePath(const std::string& name) {
        return ::testing::TempDir() + name;
    }

    json::Value Fields(const std::string& name, const std::string& role) {
        json::Value fields = json::Value::MakeObject();
        fields.Set("name", name);
        fields.Set("role", role);
        return fields;
    }

    void WriteRawFile(const std::string& path, const std::string& content) {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << content;
    }

} // namespace

TEST(JsonRecordStoreTest, CreateAssignsIncrementingIds) {
    std::string path = TempStorePath("record_store_create.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int firstId = store.Create(Fields("Alice", "engineer"));
    int secondId = store.Create(Fields("Bob", "designer"));

    EXPECT_EQ(firstId, 1);
    EXPECT_EQ(secondId, 2);
    EXPECT_EQ(store.All().size(), 2u);

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, FindReturnsMatchingRecord) {
    std::string path = TempStorePath("record_store_find.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int id = store.Create(Fields("Alice", "engineer"));

    const json::Value* found = store.Find(id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ((*found)["name"].AsString(), "Alice");
    EXPECT_EQ(store.Find(id + 1), nullptr);

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, UpdateMergesFieldsWithoutChangingId) {
    std::string path = TempStorePath("record_store_update.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int id = store.Create(Fields("Alice", "engineer"));

    json::Value patch = json::Value::MakeObject();
    patch.Set("role", "manager");
    patch.Set("id", 999); // must not overwrite the real id

    EXPECT_TRUE(store.Update(id, patch));

    const json::Value* updated = store.Find(id);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ((*updated)["name"].AsString(), "Alice");
    EXPECT_EQ((*updated)["role"].AsString(), "manager");
    EXPECT_DOUBLE_EQ((*updated)["id"].AsNumber(), static_cast<double>(id));

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, UpdateReturnsFalseWhenRecordMissing) {
    std::string path = TempStorePath("record_store_update_missing.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    EXPECT_FALSE(store.Update(1, Fields("Ghost", "none")));

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, DeleteRemovesRecord) {
    std::string path = TempStorePath("record_store_delete.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int id = store.Create(Fields("Alice", "engineer"));
    EXPECT_TRUE(store.Delete(id));
    EXPECT_EQ(store.Find(id), nullptr);
    EXPECT_FALSE(store.Delete(id));

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, PersistsAcrossInstancesAndResumesIdSequence) {
    std::string path = TempStorePath("record_store_persist.json");
    std::remove(path.c_str());

    int firstId;
    {
        crudapp::JsonRecordStore store(path);
        firstId = store.Create(Fields("Alice", "engineer"));
    }

    crudapp::JsonRecordStore reloaded(path);
    ASSERT_EQ(reloaded.All().size(), 1u);
    EXPECT_EQ(reloaded.All()[0]["name"].AsString(), "Alice");

    int secondId = reloaded.Create(Fields("Bob", "designer"));
    EXPECT_GT(secondId, firstId);

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, SaveWritesAParsableJsonArrayToDisk) {
    std::string path = TempStorePath("record_store_save_raw.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int id = store.Create(Fields("Alice", "engineer"));

    json::Value onDisk = json::Value::ParseFile(path);
    ASSERT_TRUE(onDisk.IsArray());
    ASSERT_EQ(onDisk.AsArray().size(), 1u);
    EXPECT_EQ(onDisk[static_cast<size_t>(0)]["name"].AsString(), "Alice");
    EXPECT_DOUBLE_EQ(onDisk[static_cast<size_t>(0)]["id"].AsNumber(), static_cast<double>(id));

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, LoadIgnoresMalformedEntriesInsteadOfCrashing) {
    std::string path = TempStorePath("record_store_malformed_entries.json");
    // A non-object entry and an object missing "id" should both be skipped;
    // the well-formed record should still load, and id numbering should
    // continue from the highest valid id found.
    WriteRawFile(path, R"([{"name":"NoId"},"not an object",{"name":"Alice","id":5}])");

    crudapp::JsonRecordStore store(path);

    ASSERT_EQ(store.All().size(), 1u);
    EXPECT_EQ(store.All()[0]["name"].AsString(), "Alice");

    int newId = store.Create(Fields("Bob", "designer"));
    EXPECT_GT(newId, 5);

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, LoadTreatsUnparsableFileAsEmptyStore) {
    std::string path = TempStorePath("record_store_unparsable.json");
    WriteRawFile(path, "{ this is not valid json");

    crudapp::JsonRecordStore store(path);

    EXPECT_TRUE(store.All().empty());
    EXPECT_EQ(store.Create(Fields("Alice", "engineer")), 1);

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, LoadTreatsNonArrayTopLevelValueAsEmptyStore) {
    std::string path = TempStorePath("record_store_non_array.json");
    WriteRawFile(path, R"({"name":"Alice"})");

    crudapp::JsonRecordStore store(path);

    EXPECT_TRUE(store.All().empty());

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, UpdateWithNonObjectFieldsLeavesRecordUnchanged) {
    std::string path = TempStorePath("record_store_update_non_object.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int id = store.Create(Fields("Alice", "engineer"));

    EXPECT_TRUE(store.Update(id, json::Value(42)));

    const json::Value* record = store.Find(id);
    ASSERT_NE(record, nullptr);
    EXPECT_EQ((*record)["name"].AsString(), "Alice");
    EXPECT_EQ((*record)["role"].AsString(), "engineer");

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, FindByFieldReturnsAllMatchingRecords) {
    std::string path = TempStorePath("record_store_find_by_field.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    store.Create(Fields("Alice", "engineer"));
    store.Create(Fields("Bob", "designer"));
    store.Create(Fields("Carol", "engineer"));

    json::Value::Array matches = store.FindByField("role", "engineer");
    ASSERT_EQ(matches.size(), 2u);
    EXPECT_EQ(matches[0]["name"].AsString(), "Alice");
    EXPECT_EQ(matches[1]["name"].AsString(), "Carol");

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, FindByFieldReturnsEmptyWhenNoMatch) {
    std::string path = TempStorePath("record_store_find_by_field_missing.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    store.Create(Fields("Alice", "engineer"));

    EXPECT_TRUE(store.FindByField("role", "manager").empty());
    EXPECT_TRUE(store.FindByField("nonexistent-key", "anything").empty());

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, DeleteLeavesOtherRecordsIntactOnDiskAndInMemory) {
    std::string path = TempStorePath("record_store_delete_multi.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    int firstId = store.Create(Fields("Alice", "engineer"));
    int middleId = store.Create(Fields("Bob", "designer"));
    int lastId = store.Create(Fields("Carol", "manager"));

    EXPECT_TRUE(store.Delete(middleId));

    EXPECT_EQ(store.All().size(), 2u);
    ASSERT_NE(store.Find(firstId), nullptr);
    EXPECT_EQ((*store.Find(firstId))["name"].AsString(), "Alice");
    EXPECT_EQ(store.Find(middleId), nullptr);
    ASSERT_NE(store.Find(lastId), nullptr);
    EXPECT_EQ((*store.Find(lastId))["name"].AsString(), "Carol");

    json::Value onDisk = json::Value::ParseFile(path);
    ASSERT_TRUE(onDisk.IsArray());
    EXPECT_EQ(onDisk.AsArray().size(), 2u);

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, UpdateOnlyChangesTheTargetFieldLeavingOthersIntact) {
    std::string path = TempStorePath("record_store_update_single_field.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    json::Value fields = json::Value::MakeObject();
    fields.Set("name", "Alice");
    fields.Set("role", "engineer");
    fields.Set("department", "platform");
    int id = store.Create(fields);

    json::Value patch = json::Value::MakeObject();
    patch.Set("role", "manager");

    EXPECT_TRUE(store.Update(id, patch));

    const json::Value* updated = store.Find(id);
    ASSERT_NE(updated, nullptr);
    EXPECT_EQ((*updated)["role"].AsString(), "manager");
    EXPECT_EQ((*updated)["name"].AsString(), "Alice");
    EXPECT_EQ((*updated)["department"].AsString(), "platform");

    std::remove(path.c_str());
}

TEST(JsonRecordStoreTest, FindAndDeleteReturnNotFoundForOutOfRangeIds) {
    std::string path = TempStorePath("record_store_boundary_ids.json");
    std::remove(path.c_str());
    crudapp::JsonRecordStore store(path);

    store.Create(Fields("Alice", "engineer"));

    EXPECT_EQ(store.Find(0), nullptr);
    EXPECT_EQ(store.Find(-1), nullptr);
    EXPECT_FALSE(store.Delete(0));
    EXPECT_FALSE(store.Delete(-1));

    std::remove(path.c_str());
}
