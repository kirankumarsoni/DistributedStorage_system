#pragma once
#include <string>
#include <stdexcept>

// In-memory test double for the socket-style connect/send/receive/close contract.
// It is intentionally in-memory: it verifies call sequencing without opening sockets.
class MockSocketOps
{
public:
    bool connect(const std::string &)
    {
        connected_ = true;
        return true;
    }
    std::size_t send(const std::string &bytes)
    {
        if (!connected_)
            throw std::runtime_error("mock send before connect");
        sent_ += bytes;
        return bytes.size();
    }
    std::string receive()
    {
        if (!connected_)
            throw std::runtime_error("mock receive before connect");
        return "ACK";
    }
    void close() { connected_ = false; }
    const std::string &sent() const { return sent_; }

private:
    bool connected_ = false;
    std::string sent_;
};
