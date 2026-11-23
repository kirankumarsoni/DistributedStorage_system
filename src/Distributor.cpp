#include "Distributor.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

// Stable FNV-1a partitioning (unlike std::hash, this is reproducible across runs).
static std::uint32_t fnv1a(const std::string &s)
{
    std::uint32_t h = 2166136261u;
    for (unsigned char c : s)
    {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}
Distributor::Distributor(const Config &c) : nodeCount_(c.nodes), keyType_(c.keyType) {}
int Distributor::ownerOf(const std::string &key) const
{
    std::uint32_t h;
    if (keyType_ == "int32")
    {
        char *end = nullptr;
        errno = 0;
        long long n = std::strtoll(key.c_str(), &end, 10);
        if (errno == ERANGE || end == key.c_str() || *end != '\0' || n < INT32_MIN || n > INT32_MAX)
            throw std::runtime_error("Invalid int32 key: " + key);
        h = static_cast<std::uint32_t>(static_cast<std::int32_t>(n));
    }
    else
        h = fnv1a(key);
    return static_cast<int>(h % static_cast<std::uint32_t>(nodeCount_));
}
