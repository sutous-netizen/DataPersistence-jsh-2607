#include "JsonRecordStore.h"

#include <algorithm>
#include <fstream>

namespace crudapp {

    int RecordId(const json::Value& record) {
        return static_cast<int>(record["id"].AsNumber());
    }

    JsonRecordStore::JsonRecordStore(std::string filePath) : filePath_(std::move(filePath)) {
        Load();
    }

    void JsonRecordStore::Load() {
        std::ifstream probe(filePath_, std::ios::binary);
        if (!probe.good()) {
            return;
        }
        probe.close();

        json::Value loaded;
        try {
            loaded = json::Value::ParseFile(filePath_);
        } catch (const std::exception&) {
            return; // Corrupt file: start with an empty in-memory collection.
        }
        if (!loaded.IsArray()) {
            return;
        }

        for (const auto& record : loaded.AsArray()) {
            if (!record.IsObject() || !record.Contains("id")) {
                continue; // Skip malformed entries rather than crashing.
            }
            records_.push_back(record);
            nextId_ = std::max(nextId_, RecordId(record) + 1);
        }
    }

    void JsonRecordStore::Save() const {
        json::Value(records_).SaveToFile(filePath_, 2);
    }

    int JsonRecordStore::Create(json::Value fields) {
        if (!fields.IsObject()) {
            fields = json::Value::MakeObject();
        }
        int id = nextId_++;
        fields.Set("id", id);
        records_.push_back(std::move(fields));
        Save();
        return id;
    }

    json::Value::Array::iterator JsonRecordStore::FindMutable(int id) {
        return std::find_if(records_.begin(), records_.end(),
            [id](const json::Value& record) { return RecordId(record) == id; });
    }

    const json::Value* JsonRecordStore::Find(int id) const {
        auto it = std::find_if(records_.begin(), records_.end(),
            [id](const json::Value& record) { return RecordId(record) == id; });
        return it == records_.end() ? nullptr : &(*it);
    }

    json::Value::Array JsonRecordStore::FindByField(const std::string& key, const std::string& value) const {
        json::Value::Array matches;
        for (const auto& record : records_) {
            if (record.IsObject() && record.Contains(key) && record[key].IsString() &&
                record[key].AsString() == value) {
                matches.push_back(record);
            }
        }
        return matches;
    }

    const json::Value::Array& JsonRecordStore::All() const {
        return records_;
    }

    bool JsonRecordStore::Update(int id, const json::Value& fields) {
        auto it = FindMutable(id);
        if (it == records_.end()) {
            return false;
        }

        if (fields.IsObject()) {
            for (const auto& member : fields.AsObject()) {
                if (member.first == "id") {
                    continue;
                }
                it->Set(member.first, member.second);
            }
        }

        Save();
        return true;
    }

    bool JsonRecordStore::Delete(int id) {
        auto it = FindMutable(id);
        if (it == records_.end()) {
            return false;
        }

        records_.erase(it);
        Save();
        return true;
    }

} // namespace crudapp
