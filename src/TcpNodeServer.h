#pragma once
#include "Node.h"
#include "Distributor.h"
#include "SocketPlatform.h"
#include <atomic>
#include <thread>
#include <string>

class TcpNodeServer
{
public:
    TcpNodeServer(Node &node, int port, const Distributor &distributor, const std::string &listenAddress = "0.0.0.0");
    ~TcpNodeServer();
    void start();
    void stop();
    TcpNodeServer(const TcpNodeServer &) = delete;
    TcpNodeServer &operator=(const TcpNodeServer &) = delete;

private:
    void run();
    void handleClient(int client);
    Node &node_;
    const Distributor &distributor_;
    int port_;
    std::string listenAddress_;
    int listenSocket_ = InvalidSocket;
    std::thread worker_;
    std::atomic<bool> stopping_{false};
};
