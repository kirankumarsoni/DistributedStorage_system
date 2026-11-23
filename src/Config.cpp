#include "Config.h"
#include <fstream>
#include <stdexcept>

Config Config::load(const std::string &path)
{
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Cannot open config file: " + path);

    Config c;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        const std::size_t p = line.find('=');
        if (p == std::string::npos)
            continue;

        const std::string key = line.substr(0, p);
        const std::string value = line.substr(p + 1);
        if (key == "nodes")
            c.nodes = std::stoi(value);
        else if (key == "key_type")
            c.keyType = value;
        else if (key == "value_type")
            c.valueType = value;
        else if (key == "delimiter")
        {
            if (value.size() != 1)
                throw std::runtime_error("delimiter must be one character");
            c.delimiter = value[0];
        }
        else if (key == "batch_size")
            c.batchSize = std::stoull(value);
        else if (key == "base_port")
            c.basePort = std::stoi(value);
        else if (key == "node_host")
            c.nodeHost = value;
        else if (key == "listen_address")
            c.listenAddress = value;
    }

    if (c.nodes < 1 || c.nodes > 5)
        throw std::runtime_error("nodes must be between 1 and 5");
    if (c.keyType != "int32" && c.keyType != "string")
        throw std::runtime_error("key_type must be int32 or string");
    if (c.valueType != "int32" && c.valueType != "string")
        throw std::runtime_error("value_type must be int32 or string");
    if (c.batchSize == 0 || c.batchSize > 1000000)
        throw std::runtime_error("batch_size must be between 1 and 1000000");
    if (c.basePort < 1024 || c.basePort > 65530 || c.basePort + c.nodes > 65535)
        throw std::runtime_error("base_port must leave a valid TCP port for every node (1024..65530)");
    return c;
}
