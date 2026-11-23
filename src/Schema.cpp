#include "Schema.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>

Schema::Schema(const Config &config)
    : keyType_(config.keyType), valueType_(config.valueType) {}

bool Schema::isInt32(const std::string &value)
{
    if (value.empty())
        return false;

    char *end = nullptr;
    errno = 0;
    const long long number = std::strtoll(value.c_str(), &end, 10);
    return errno != ERANGE && end != value.c_str() && *end == '\0' &&
           number >= INT32_MIN && number <= INT32_MAX;
}

bool Schema::validate(const Record &record, std::string &reason) const
{
    if (record.key.empty() || record.value.empty())
    {
        reason = "key and value must not be empty";
        return false;
    }
    if (keyType_ == "int32" && !isInt32(record.key))
    {
        reason = "key is not a valid int32";
        return false;
    }
    if (valueType_ == "int32" && !isInt32(record.value))
    {
        reason = "value is not a valid int32";
        return false;
    }
    return true;
}
