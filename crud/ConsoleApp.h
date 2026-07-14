#pragma once

#include <iostream>
#include <string>

#include "IRecordStore.h"

namespace crudapp {

    // Interactive text menu (Create/Read/Update/Delete) over an IRecordStore.
    // Depending on the interface (rather than JsonRecordStore directly) keeps
    // the menu logic testable against a mock and reusable with any future
    // storage backend. The stream parameters default to std::cin/std::cout but
    // can be swapped for an istringstream/ostringstream in tests.
    class ConsoleApp {
    public:
        explicit ConsoleApp(IRecordStore& store, std::istream& in = std::cin,
            std::ostream& out = std::cout);

        // Runs the menu loop until the user exits or input ends. Returns a
        // process exit code (always 0; reserved for future use).
        int Run();

    private:
        void PrintMenu();
        void HandleCreate();
        void HandleReadAll();
        void HandleReadOne();
        void HandleUpdate();
        void HandleDelete();

        // Reads "key=value" lines until a blank line, returning them as a JSON object.
        json::Value ReadFieldsFromInput();
        // Reads a line and parses it as an integer id; returns -1 on failure.
        int ReadIdFromInput();
        // Parses a full string as an integer id; returns -1 on failure.
        int ParseId(const std::string& text) const;

        IRecordStore& store_;
        std::istream& in_;
        std::ostream& out_;
    };

} // namespace crudapp
