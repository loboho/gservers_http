#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#pragma comment(lib,"ws2_32.lib")
#endif
#include "gbase/httpquery.h"

int main()
{
#if defined(_WIN32) || defined(_WIN64)
    WSADATA data;
    WORD version = MAKEWORD(2, 2);
    auto ret = WSAStartup(version, &data);
#endif // DEBUG

    GHttpSender http;
    std::cout << "============== Send Post =============" << std::endl;
    auto checkResult = http.HttpPost("www.china.com", 80, "/", "test", 4);
    auto err = GetLastError();
    GHttpSender::tResultVector getRs;
    while (!checkResult(getRs))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    std::cout << "============= Post Result ============" << std::endl;
    for (auto pBuf : getRs)
    {
        std::cout << (const char*)pBuf->GetData();
    }
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
#endif // DEBUG
    return 0;
}

