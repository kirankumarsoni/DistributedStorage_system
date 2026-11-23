#include "MockSocketOps.h"
#include <iostream>
int main()
{
    MockSocketOps mock;
    if (!mock.connect("127.0.0.1:21000"))
        return 1;
    mock.send("sample-record");
    if (mock.receive() != "ACK" || mock.sent() != "sample-record")
        return 2;
    mock.close();
    std::cout << "MOCK SOCKET TEST PASS: connect/send/receive/close sequence verified (no real network used).\n";
    return 0;
}
