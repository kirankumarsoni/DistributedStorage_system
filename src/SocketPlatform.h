#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

constexpr int InvalidSocket = -1;

inline void closeSocket(int socketHandle)
{
    if (socketHandle != InvalidSocket)
        ::close(socketHandle);
}

inline void setSocketTimeouts(int socketHandle, int seconds)
{
    timeval timeout{};
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
               &timeout, sizeof(timeout));
    setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
               &timeout, sizeof(timeout));
}
