#include "TcpTransport.h"
#include "SocketPlatform.h"
#include "Log.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <chrono>
#include <thread>
#include <utility>

namespace
{
    bool sendAll(int s, const void *data, std::size_t bytes)
    {
        const char *p = static_cast<const char *>(data);
        while (bytes)
        {
            ssize_t n = send(s, p, bytes, 0);
            if (n <= 0)
                return false;
            p += n;
            bytes -= static_cast<std::size_t>(n);
        }
        return true;
    }
    bool receiveAll(int s, void *data, std::size_t bytes)
    {
        char *p = static_cast<char *>(data);
        while (bytes)
        {
            ssize_t n = recv(s, p, bytes, 0);
            if (n <= 0)
                return false;
            p += n;
            bytes -= static_cast<std::size_t>(n);
        }
        return true;
    }
    void writeU32(int s, std::uint32_t v)
    {
        v = htonl(v);
        if (!sendAll(s, &v, sizeof(v)))
            throw std::runtime_error("TCP send failed");
    }
    void writeString(int s, const std::string &value)
    {
        if (value.size() > 64u * 1024u * 1024u)
            throw std::runtime_error("record field exceeds 64 MiB");
        writeU32(s, static_cast<std::uint32_t>(value.size()));
        if (!value.empty() && !sendAll(s, value.data(), value.size()))
            throw std::runtime_error("TCP field send failed");
    }
}

TcpTransport::TcpTransport(const Config &config) : basePort_(config.basePort), nodeCount_(config.nodes), nodeHost_(config.nodeHost) {}

void TcpTransport::sendBatch(int destinationId, std::vector<Record> &records)
{
    if (destinationId < 0 || destinationId >= nodeCount_)
        throw std::out_of_range("invalid TCP destination node");
    if (records.empty())
        return;
    if (records.size() > 1000000)
        throw std::runtime_error("TCP batch exceeds one million records");

    int client = InvalidSocket;
    try
    {
        client = ::socket(AF_INET, SOCK_STREAM, 0);
        if (client == InvalidSocket)
            throw std::runtime_error("client socket() failed");
        setSocketTimeouts(client, 30);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        if (inet_pton(AF_INET, nodeHost_.c_str(), &address.sin_addr) != 1)
            throw std::runtime_error("node_host must be an IPv4 address: " + nodeHost_);
        address.sin_port = htons(static_cast<std::uint16_t>(basePort_ + destinationId));
        const int destinationPort = basePort_ + destinationId;
        logMessage("[CLIENT] Connecting to Node " + std::to_string(destinationId) +
                   " at " + nodeHost_ + ":" + std::to_string(destinationPort));
        bool connected = false;
        for (int attempt = 1; attempt <= 20; ++attempt)
        {
            if (::connect(client, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0)
            {
                connected = true;
                break;
            }
            closeSocket(client);
            client = InvalidSocket;
            if (attempt < 20)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                client = ::socket(AF_INET, SOCK_STREAM, 0);
                if (client == InvalidSocket)
                    throw std::runtime_error("client socket() retry failed");
                setSocketTimeouts(client, 30);
            }
        }
        if (!connected)
            throw std::runtime_error("connect() failed after retries for node " + std::to_string(destinationId) +
                                     " on port " + std::to_string(destinationPort));
        logMessage("[CLIENT] Connected to Node " + std::to_string(destinationId) +
                   ". Sending batch of " + std::to_string(records.size()) + " records.");
        writeU32(client, static_cast<std::uint32_t>(records.size()));
        for (const Record &record : records)
        {
            writeString(client, record.key);
            writeString(client, record.value);
        }
        std::uint32_t ack = 0;
        if (!receiveAll(client, &ack, sizeof(ack)) || ntohl(ack) != records.size())
            throw std::runtime_error("node " + std::to_string(destinationId) + " returned invalid batch ACK");
        logMessage("[CLIENT] ACK received from Node " + std::to_string(destinationId) +
                   ": " + std::to_string(ntohl(ack)) + " records accepted.");
        closeSocket(client);
        logMessage("[CLIENT] Connection to Node " + std::to_string(destinationId) + " closed.");
        records.clear(); // Reuse the caller's reserved capacity for the next batch.
    }
    catch (...)
    {
        closeSocket(client);
        throw;
    }
}

NodeInventory TcpTransport::fetchInventory(int nodeId)
{
    if (nodeId < 0 || nodeId >= nodeCount_)
        throw std::out_of_range("invalid audit node");
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == InvalidSocket)
        throw std::runtime_error("audit socket() failed");
    try
    {
        setSocketTimeouts(s, 30);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        if (inet_pton(AF_INET, nodeHost_.c_str(), &a.sin_addr) != 1)
            throw std::runtime_error("invalid node_host");
        a.sin_port = htons(static_cast<std::uint16_t>(basePort_ + nodeId));
        if (::connect(s, reinterpret_cast<sockaddr *>(&a), sizeof(a)) != 0)
            throw std::runtime_error("audit connect failed for node " + std::to_string(nodeId));
        std::uint32_t zero = 0;
        if (!sendAll(s, &zero, sizeof(zero)))
            throw std::runtime_error("audit request failed");
        std::uint32_t received = 0, n = 0;
        if (!receiveAll(s, &received, sizeof(received)) || !receiveAll(s, &n, sizeof(n)))
            throw std::runtime_error("audit response failed");
        received = ntohl(received);
        n = ntohl(n);
        if (n > 100000000u)
            throw std::runtime_error("audit inventory too large");
        NodeInventory inventory;
        inventory.receivedRecords = received;
        inventory.keys.reserve(n);
        for (std::uint32_t i = 0; i < n; ++i)
        {
            std::uint32_t len = 0;
            if (!receiveAll(s, &len, sizeof(len)))
                throw std::runtime_error("audit key length failed");
            len = ntohl(len);
            if (len > 64u * 1024u * 1024u)
                throw std::runtime_error("audit key too large");
            std::string k(len, '\0');
            if (len && !receiveAll(s, &k[0], len))
                throw std::runtime_error("audit key read failed");
            inventory.keys.push_back(std::move(k));
        }
        closeSocket(s);
        return inventory;
    }
    catch (...)
    {
        closeSocket(s);
        throw;
    }
}
