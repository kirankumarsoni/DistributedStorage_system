#pragma once
#include <cstddef>
#include <string>

struct Config
{
    int nodes = 3;
    std::string keyType = "int32";
    std::string valueType = "string";
    char delimiter = ',';
    std::size_t batchSize = 4096;
    int basePort = 19000;
    std::string nodeHost = "127.0.0.1";
    std::string listenAddress = "0.0.0.0";

    static Config load(const std::string &path);
};
