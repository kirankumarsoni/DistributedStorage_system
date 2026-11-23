#include "TcpNodeServer.h"
#include "Log.h"
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace
{
    bool receiveAll(int s, void *data, std::size_t bytes)
    {
        char *p = static_cast<char *>(data);
        while (bytes)
        {
            ssize_t n = recv(s, p, bytes, 0);
            if (n == 0)
                return false;
            if (n < 0)
                return false;
            p += n;
            bytes -= static_cast<std::size_t>(n);
        }
        return true;
    }
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
    std::uint32_t readU32(int s)
    {
        std::uint32_t v;
        if (!receiveAll(s, &v, sizeof(v)))
            throw std::runtime_error("incomplete TCP frame");
        return ntohl(v);
    }
    void writeU32(int s, std::uint32_t v)
    {
        v = htonl(v);
        if (!sendAll(s, &v, sizeof(v)))
            throw std::runtime_error("TCP acknowledgement failed");
    }
    std::string readString(int s)
    {
        const std::uint32_t length = readU32(s);
        constexpr std::uint32_t MaxFieldBytes = 64u * 1024u * 1024u;
        if (length > MaxFieldBytes)
            throw std::runtime_error("TCP field exceeds 64 MiB safety limit");
        std::string value(length, '\0');
        if (length && !receiveAll(s, &value[0], length))
            throw std::runtime_error("incomplete TCP field");
        return value;
    }
}

TcpNodeServer::TcpNodeServer(Node &node, int port, const Distributor &distributor, const std::string &listenAddress)
    : node_(node), distributor_(distributor), port_(port), listenAddress_(listenAddress) {}
TcpNodeServer::~TcpNodeServer() { stop(); }

void TcpNodeServer::start()
{
    if (worker_.joinable())
        return;
    listenSocket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ == InvalidSocket)
        throw std::runtime_error("node " + std::to_string(node_.id()) + ": socket() failed");
    int reuse = 1;
    setsockopt(listenSocket_, SOL_SOCKET, SO_REUSEADDR,
               &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    if (inet_pton(AF_INET, listenAddress_.c_str(), &address.sin_addr) != 1)
        throw std::runtime_error("invalid listen address: " + listenAddress_);
    address.sin_port = htons(static_cast<std::uint16_t>(port_));
    if (::bind(listenSocket_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0)
    {
        closeSocket(listenSocket_);
        listenSocket_ = InvalidSocket;
        throw std::runtime_error("node " + std::to_string(node_.id()) + ": bind failed on port " + std::to_string(port_));
    }
    if (::listen(listenSocket_, 64) != 0)
    {
        closeSocket(listenSocket_);
        listenSocket_ = InvalidSocket;
        throw std::runtime_error("listen() failed on port " + std::to_string(port_));
    }
    stopping_ = false;
    worker_ = std::thread(&TcpNodeServer::run, this);
    logMessage("[SERVER] Node " + std::to_string(node_.id()) +
               " listening on " + listenAddress_ + ":" + std::to_string(port_));
}

void TcpNodeServer::stop()
{
    stopping_ = true;
    if (listenSocket_ != InvalidSocket)
    {
        shutdown(listenSocket_, SHUT_RDWR);
        closeSocket(listenSocket_);
        listenSocket_ = InvalidSocket;
    }
    if (worker_.joinable())
        worker_.join();
}

void TcpNodeServer::run()
{
    while (!stopping_)
    {
        sockaddr_in peer{};
        socklen_t peerLength = sizeof(peer);
        int client = accept(listenSocket_, reinterpret_cast<sockaddr *>(&peer), &peerLength);
        if (client == InvalidSocket)
        {
            if (stopping_)
                break;
            continue;
        }
        setSocketTimeouts(client, 30);
        logMessage("[SERVER] Node " + std::to_string(node_.id()) + " accepted a TCP connection.");
        try
        {
            handleClient(client);
        }
        catch (const std::exception &error)
        {
            logMessage("[SERVER] Node " + std::to_string(node_.id()) +
                       " rejected connection: " + error.what());
        }
        catch (...)
        {
            logMessage("[SERVER] Node " + std::to_string(node_.id()) +
                       " rejected connection: unknown error.");
        }
        closeSocket(client);
        logMessage("[SERVER] Node " + std::to_string(node_.id()) + " closed client connection.");
    }
}

void TcpNodeServer::handleClient(int client)
{
    const std::uint32_t count = readU32(client);
    // Control frame: zero requests an inventory for independent ownership audit.
    if (count == 0)
    {
        const auto keys = node_.keys();
        writeU32(client, static_cast<std::uint32_t>(node_.receivedCount()));
        writeU32(client, static_cast<std::uint32_t>(keys.size()));
        for (const auto &key : keys)
        {
            writeU32(client, static_cast<std::uint32_t>(key.size()));
            if (!key.empty() && !sendAll(client, key.data(), key.size()))
                throw std::runtime_error("inventory send failed");
        }
        return;
    }
    constexpr std::uint32_t MaxBatchRecords = 1000000;
    if (count > MaxBatchRecords)
        throw std::runtime_error("invalid TCP batch record count");
    logMessage("[SERVER] Node " + std::to_string(node_.id()) +
               " receiving batch: " + std::to_string(count) + " records.");
    std::vector<Record> batch;
    batch.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i)
        batch.push_back(Record{readString(client), readString(client)});
    for (const Record &record : batch)
    {
        if (distributor_.ownerOf(record.key) != node_.id())
            throw std::runtime_error("record key routed to the wrong node: " + record.key);
    }
    node_.receiveBatch(batch);
    logMessage("[SERVER] Node " + std::to_string(node_.id()) +
               " stored batch. Received total: " + std::to_string(node_.receivedCount()) +
               ", unique keys: " + std::to_string(node_.uniqueCount()) + ".");
    writeU32(client, count); // ACK only after the batch has been committed to node storage.
    logMessage("[SERVER] Node " + std::to_string(node_.id()) +
               " sent ACK: " + std::to_string(count) + " records.");
}
