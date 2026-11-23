#pragma once

#include <iostream>
#include <mutex>
#include <string>

// Keep messages from the client thread and node-server threads readable.
inline void logMessage(const std::string &message)
{
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << message << std::endl;
}
