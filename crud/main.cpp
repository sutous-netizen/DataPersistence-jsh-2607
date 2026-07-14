#include <iostream>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "Json.h"

namespace {

    void RunJsonDemo() {
        json::Value root = json::Value::MakeObject();
        root.Set("name", "crud");
        root.Set("version", 1);
        root.Set("active", true);

        json::Value tags = json::Value::MakeArray();
        tags.PushBack("json");
        tags.PushBack("library");
        root.Set("tags", tags);

        std::cout << root.ToString(2) << std::endl;

        root.SaveToFile("output.json", 2);

        json::Value loaded = json::Value::ParseFile("output.json");
        std::cout << "name: " << loaded["name"].AsString() << std::endl;
    }

} // namespace

int main(int argc, char** argv) {
    RunJsonDemo();

    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
