#pragma once
#include "Config.h"
#include "Record.h"
#include <string>
class Schema
{
public:
    explicit Schema(const Config &);
    bool validate(const Record &, std::string &) const;

private:
    std::string keyType_, valueType_;
    static bool isInt32(const std::string &);
};
