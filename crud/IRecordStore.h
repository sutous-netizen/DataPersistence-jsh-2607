#pragma once

#include <string>

#include "Json.h"

namespace crudapp {

    // Abstraction over a Create/Read/Update/Delete record collection, so
    // consumers (e.g. ConsoleApp) don't depend on a specific storage backend
    // and can be tested against a mock instead of real file I/O.
    class IRecordStore {
    public:
        virtual ~IRecordStore() = default;

        virtual int Create(json::Value fields) = 0;
        virtual const json::Value* Find(int id) const = 0;
        // Returns every record whose string field `key` equals `value`.
        virtual json::Value::Array FindByField(const std::string& key, const std::string& value) const = 0;
        virtual const json::Value::Array& All() const = 0;
        virtual bool Update(int id, const json::Value& fields) = 0;
        virtual bool Delete(int id) = 0;
    };

    // Extracts the integer "id" field from a record. Shared by IRecordStore
    // implementations and by consumers (e.g. ConsoleApp's sort-by-id listing).
    int RecordId(const json::Value& record);

} // namespace crudapp
