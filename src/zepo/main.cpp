//
// Created by qingy on 2024/7/14.
//

#include <array>
#include <chrono>
#include <iostream>
#include <fstream>

#include "async/AsyncIO.hpp"
#include "async/Task.hpp"
#include "zepo/serialize/Reflect.hpp"
#include "zepo/serialize/Serializer.hpp"
#include "zepo/serialize/Json.hpp"

struct Package {
    std::string name;
    std::map<std::string, std::string> dependencies;
    std::map<std::string, std::string> devDependencies;
    std::string version;
};

ZEPO_REFLECT_INFO_BEGIN_(Package)
    ZEPO_REFLECT_FIELD_(name);
    ZEPO_REFLECT_FIELD_(dependencies);
    ZEPO_REFLECT_FIELD_(devDependencies);
    ZEPO_REFLECT_FIELD_(version);
ZEPO_REFLECT_INFO_END_()

ZEPO_REFLECT_PARSABLE_(Package);
ZEPO_REFLECT_ACCESSABLE_(Package);

using namespace zepo;

Task<int> asyncMain() {
    using namespace std::chrono;

    std::fstream packageJson{"package.json", std::ios::out | std::ios::in};
    if (!packageJson.good()) {
        co_return 1;
    }

    std::array<char, 1024> content{};
    auto readSize = co_await async_io::readStream(packageJson, content);

    const auto begin = high_resolution_clock::now();

    try {
        auto jsonDoc = JsonDocument{std::string_view(content.data(), readSize )};
        auto object = zepo::parse<Package>(jsonDoc.getRootToken());

        const auto end = high_resolution_clock::now();

        std::cout << "time: " << duration_cast<microseconds>(end - begin).count() << "us" << "\n";

        std::cout << object.name << "\n";
        zepo::MetadataOf<Package>.setField<std::string>(object, "name", "hello-world");
        std::cout << object.name << "\n";
    } catch (std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
    }

    co_return 0;
}

int main() {
    auto mainTask = asyncMain();
    return mainTask.getValue();
}
