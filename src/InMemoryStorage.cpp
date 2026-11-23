#include "InMemoryStorage.h"
#include <utility>

void InMemoryStorage::putBatch(std::vector<Record> &records)
{
    for (Record &record : records)
    {
        auto result = records_.try_emplace(std::move(record.key), std::move(record.value));
        if (!result.second)
            result.first->second = std::move(record.value);
    }
    records.clear(); // Retains vector capacity for the next batch.
}

std::size_t InMemoryStorage::size() const { return records_.size(); }

std::size_t InMemoryStorage::payloadBytes() const
{
    std::size_t bytes = 0;
    for (const auto &item : records_)
        bytes += item.first.size() + item.second.size();
    return bytes;
}

const std::unordered_map<std::string, std::string> &InMemoryStorage::all() const
{
    return records_;
}
