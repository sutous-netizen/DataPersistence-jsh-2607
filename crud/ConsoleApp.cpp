#include "ConsoleApp.h"

#include <exception>
#include <string>

#include "QuickSort.h"

namespace crudapp {

    ConsoleApp::ConsoleApp(IRecordStore& store, std::istream& in, std::ostream& out)
        : store_(store), in_(in), out_(out) {
    }

    void ConsoleApp::PrintMenu() {
        out_ << "\n=== CRUD Console ===\n"
             << "1. Create\n"
             << "2. Read all\n"
             << "3. Read one (by id or key=value)\n"
             << "4. Update\n"
             << "5. Delete\n"
             << "6. Exit\n"
             << "Select: ";
    }

    int ConsoleApp::Run() {
        while (true) {
            PrintMenu();
            std::string choice;
            if (!std::getline(in_, choice)) {
                return 0;
            }

            if (choice == "1") {
                HandleCreate();
            } else if (choice == "2") {
                HandleReadAll();
            } else if (choice == "3") {
                HandleReadOne();
            } else if (choice == "4") {
                HandleUpdate();
            } else if (choice == "5") {
                HandleDelete();
            } else if (choice == "6") {
                return 0;
            } else {
                out_ << "Unknown option: " << choice << "\n";
            }
        }
    }

    json::Value ConsoleApp::ReadFieldsFromInput() {
        out_ << "Enter fields as key=value, one per line, blank line to finish:\n";
        json::Value fields = json::Value::MakeObject();
        std::string line;
        while (std::getline(in_, line) && !line.empty()) {
            auto separator = line.find('=');
            if (separator == std::string::npos) {
                out_ << "Ignoring invalid line (expected key=value): " << line << "\n";
                continue;
            }
            fields.Set(line.substr(0, separator), line.substr(separator + 1));
        }
        return fields;
    }

    int ConsoleApp::ParseId(const std::string& text) const {
        try {
            size_t consumed = 0;
            int id = std::stoi(text, &consumed);
            return consumed == text.size() ? id : -1;
        } catch (const std::exception&) {
            return -1;
        }
    }

    int ConsoleApp::ReadIdFromInput() {
        out_ << "Id: ";
        std::string line;
        std::getline(in_, line);
        return ParseId(line);
    }

    void ConsoleApp::HandleCreate() {
        json::Value fields = ReadFieldsFromInput();
        int id = store_.Create(std::move(fields));
        out_ << "Created record with id " << id << "\n";
    }

    void ConsoleApp::HandleReadAll() {
        json::Value::Array records = store_.All();
        sortlib::QuickSort(records.begin(), records.end(),
            [](const json::Value& a, const json::Value& b) { return RecordId(a) < RecordId(b); });

        if (records.empty()) {
            out_ << "(no records)\n";
            return;
        }
        for (const auto& record : records) {
            out_ << record.ToString(2) << "\n";
        }
    }

    void ConsoleApp::HandleReadOne() {
        out_ << "Enter an id, or key=value to search by field: ";
        std::string line;
        std::getline(in_, line);

        auto separator = line.find('=');
        if (separator != std::string::npos) {
            std::string key = line.substr(0, separator);
            std::string value = line.substr(separator + 1);
            json::Value::Array matches = store_.FindByField(key, value);
            if (matches.empty()) {
                out_ << "No records with " << key << "=" << value << "\n";
                return;
            }
            for (const auto& record : matches) {
                out_ << record.ToString(2) << "\n";
            }
            return;
        }

        int id = ParseId(line);
        const json::Value* record = store_.Find(id);
        if (!record) {
            out_ << "No record with id " << id << "\n";
            return;
        }
        out_ << record->ToString(2) << "\n";
    }

    void ConsoleApp::HandleUpdate() {
        int id = ReadIdFromInput();
        json::Value fields = ReadFieldsFromInput();
        if (!store_.Update(id, fields)) {
            out_ << "No record with id " << id << "\n";
            return;
        }
        out_ << "Updated record with id " << id << "\n";
    }

    void ConsoleApp::HandleDelete() {
        int id = ReadIdFromInput();
        if (!store_.Delete(id)) {
            out_ << "No record with id " << id << "\n";
            return;
        }
        out_ << "Deleted record with id " << id << "\n";
    }

} // namespace crudapp
