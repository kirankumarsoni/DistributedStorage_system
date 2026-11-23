#pragma once
#include "Record.h"
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

class InMemoryStorage
{
public:
    void putBatch(std::vector<Record> &records);
    std::size_t size() const;
    std::size_t payloadBytes() const;
    const std::unordered_map<std::string, std::string> &all() const;

private:
    std::unordered_map<std::string, std::string> records_;
};
