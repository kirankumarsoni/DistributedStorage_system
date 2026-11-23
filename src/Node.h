#pragma once
#include "InMemoryStorage.h"
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>
class Node
{
public:
    explicit Node(int id);
    int id() const;
    void receiveBatch(std::vector<Record> &records);
    std::size_t uniqueCount() const;
    std::size_t receivedCount() const;
    std::size_t payloadBytes() const;
    std::vector<std::string> keys() const;

private:
    int id_;
    mutable std::mutex mutex_;
    InMemoryStorage storage_;
    std::size_t received_ = 0;
};
