#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <cstdint>

#include <globals.h>

#pragma comment(lib, "ws2_32.lib")

namespace module::rbx
{
    struct c_server
    {
        void initialize();
        void tcp_server();
        bool read_exact_socket(SOCKET socket, void* buffer, size_t size);
    };

    inline c_server server{ };
}