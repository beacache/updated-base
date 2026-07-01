#include "server.h"
#include <task_scheduler.h>

namespace module::rbx
{
    bool c_server::read_exact_socket(SOCKET socket, void* buffer, size_t size)
    {
        size_t total = 0;
        char* out = static_cast<char*>(buffer);

        while (total < size)
        {
            int received = recv(socket, out + total, static_cast<int>(size - total), 0);
            if (received <= 0)
                return false;
            total += static_cast<size_t>(received);
        }

        return true;
    }

    void c_server::tcp_server()
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            return;

        SOCKET listen_socket = INVALID_SOCKET;
        SOCKET client_socket = INVALID_SOCKET;

        addrinfo hints{ };
        addrinfo* result = nullptr;

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(xorstr("127.0.0.1"), xorstr("6969"), &hints, &result) != 0)
        {
            WSACleanup();
            return;
        }

        listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listen_socket == INVALID_SOCKET)
        {
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        BOOL opt = TRUE;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

        if (bind(listen_socket, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR)
        {
            closesocket(listen_socket);
            freeaddrinfo(result);
            WSACleanup();
            return;
        }

        freeaddrinfo(result);

        if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR)
        {
            closesocket(listen_socket);
            WSACleanup();
            return;
        }

        while (true)
        {
            client_socket = accept(listen_socket, nullptr, nullptr);
            if (client_socket == INVALID_SOCKET)
            {
                Sleep(100);
                continue;
            }

            uint32_t net_len = 0;
            if (!read_exact_socket(client_socket, &net_len, sizeof(net_len)))
            {
                closesocket(client_socket);
                continue;
            }

            uint32_t script_len = ntohl(net_len);
            if (script_len == 0 || script_len > (8 * 1024 * 1024))
            {
                closesocket(client_socket);
                continue;
            }

            std::vector<char> buffer(script_len);
            if (!read_exact_socket(client_socket, buffer.data(), script_len))
            {
                closesocket(client_socket);
                continue;
            }

            std::string script(buffer.data(), buffer.size());
            g_task_scheduler.queue_execution(script);

            closesocket(client_socket);
        }

        closesocket(listen_socket);
        WSACleanup();
    }

    void c_server::initialize()
    {
        std::thread(&c_server::tcp_server, this).detach();
    }
}