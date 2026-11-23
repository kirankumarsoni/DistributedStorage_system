#pragma once
#include "Config.h"
#include "Distributor.h"
#include "TcpTransport.h"
#include "Schema.h"
#include "Node.h"
#include <cstddef>
#include <string>
struct LoadStats
{
    std::size_t rowsRead = 0, invalidRows = 0, fileBytes = 0, batchesSent = 0;
};
class DataLoader
{
public:
    DataLoader(const Config &, const Schema &, const Distributor &, TcpTransport &, Node *localNode = nullptr);
    LoadStats loadFile(const std::string &path, int sourceNodeId);

private:
    Config config_;
    const Schema &schema_;
    const Distributor &distributor_;
    TcpTransport &transport_;
    Node *localNode_;
};
