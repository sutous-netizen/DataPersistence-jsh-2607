#pragma once

#include <string>

#include "IRecordStore.h"
#include "Json.h"

namespace crudapp {

    // Stores a collection of JSON object records, each with a unique integer
    // "id" field, backed by a single JSON array file. Every mutation persists
    // the full collection to disk immediately, so the file always reflects the
    // in-memory state.
    class JsonRecordStore : public IRecordStore {
    public:
        // Loads existing records from filePath if it exists; otherwise starts
        // empty. Malformed entries (not an object, or missing "id") are
        // skipped; a corrupt/non-array file is treated as an empty store.
        explicit JsonRecordStore(std::string filePath);

        // Adds a new record with the given fields, assigning it a fresh id.
        // Any "id" field already present in `fields` is overwritten. Returns the
        // assigned id.
        int Create(json::Value fields) override;

        // Returns a pointer to the record with the given id, or nullptr if none
        // exists. The pointer is invalidated by any subsequent mutation.
        const json::Value* Find(int id) const override;

        // Returns every record whose string field `key` equals `value`.
        // Records without that field, or where it isn't a string, don't match.
        json::Value::Array FindByField(const std::string& key, const std::string& value) const override;

        // Returns all records, in storage order.
        const json::Value::Array& All() const override;

        // Merges `fields` into the record with the given id: existing keys are
        // overwritten and new keys are added, but "id" is never changed. Returns
        // false if no record with that id exists.
        bool Update(int id, const json::Value& fields) override;

        // Removes the record with the given id. Returns false if not found.
        bool Delete(int id) override;

    private:
        void Load();
        void Save() const;
        json::Value::Array::iterator FindMutable(int id);

        std::string filePath_;
        json::Value::Array records_;
        int nextId_ = 1;
    };

} // namespace crudapp
