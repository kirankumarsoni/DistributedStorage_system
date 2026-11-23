#pragma once
#include "Config.h"
#include <string>
class Distributor
{
public:
    explicit Distributor(const Config &);
    int ownerOf(const std::string &) const;

private:
    int nodeCount_;
    std::string keyType_;
};
