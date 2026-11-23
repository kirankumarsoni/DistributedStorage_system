#include "Config.h"
#include "Distributor.h"
#include "Node.h"
#include "TcpNodeServer.h"
#include "Log.h"
#include "Schema.h"
#include "DataLoader.h"
#include "TcpTransport.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <string>
#include <stdexcept>

namespace
{
    std::atomic<bool> stopRequested{false};

    void onSignal(int)
    {
        stopRequested = true;
    }
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: node_server <node_id>\n";
            return 2;
        }

        const int nodeId = std::stoi(argv[1]);

        const Config config = Config::load("config.conf");

        if (nodeId < 0 || nodeId >= config.nodes)
            throw std::runtime_error(
                "node_id outside configured cluster");

        Node node(nodeId);

        const Distributor distributor(config);

        TcpNodeServer server(
            node,
            config.basePort + nodeId,
            distributor,
            config.listenAddress);

        std::signal(SIGINT, onSignal);
        std::signal(SIGTERM, onSignal);

        server.start();

        logMessage(
            "[NODE] Independent process started. node_id=" +
            std::to_string(nodeId) +
            ", pid is owned by this OS process.");

        // --------------------------------------------------
        // LOCAL DATA LOADING
        // --------------------------------------------------

        const std::string localFile =
            "data/node" + std::to_string(nodeId) + ".csv";

        std::this_thread::sleep_for(
            std::chrono::seconds(3));

        try
        {
            Schema schema(config);

            TcpTransport transport(config);

            DataLoader loader(
                config,
                schema,
                distributor,
                transport,
                &node);

            const LoadStats stats =
                loader.loadFile(localFile, nodeId);

            logMessage(
                "[LOCAL-LOAD] Node " +
                std::to_string(nodeId) +
                " read " + localFile +
                " | bytes=" + std::to_string(stats.fileBytes) +
                " | rows=" + std::to_string(stats.rowsRead) +
                " | invalid=" + std::to_string(stats.invalidRows) +
                " | duplicate-policy=last-write-wins" +
                " | batches=" + std::to_string(stats.batchesSent));
        }
        catch (const std::exception &e)
        {
            logMessage(
                "[LOCAL-LOAD-ERROR] Node " +
                std::to_string(nodeId) + ": " + e.what());
        }

        // --------------------------------------------------
        // WAIT FOR SHUTDOWN
        // --------------------------------------------------

        while (!stopRequested)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(200));
        }

        // Stop the server before reporting final statistics.
        server.stop();

        // --------------------------------------------------
        // FINAL STATISTICS
        // --------------------------------------------------

        std::cout
            << "\n=== NODE " << nodeId
            << " FINAL STATISTICS ===\n"
            << "Received records: " << node.receivedCount() << "\n"
            << "Unique keys: " << node.uniqueCount() << "\n"
            << "Key/value payload estimate: "
            << node.payloadBytes() << " bytes\n";

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr
            << "NODE SERVER ERROR: "
            << e.what() << '\n';

        return 1;
    }
}
