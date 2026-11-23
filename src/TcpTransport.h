#pragma once
#include "Config.h"
#include "Record.h"
#include <vector>
struct NodeInventory { std::size_t receivedRecords=0; std::vector<std::string> keys; };
class TcpTransport {
public:
    explicit TcpTransport(const Config& config);
    void sendBatch(int destinationId, std::vector<Record>& records);
    NodeInventory fetchInventory(int nodeId);
private:
    int basePort_; int nodeCount_; std::string nodeHost_;
};
