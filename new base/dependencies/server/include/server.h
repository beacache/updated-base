#pragma once
#ifndef SRV_SOCKET_H
#define SRV_SOCKET_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

class srv_socket {
public:
    srv_socket() : listen_socket(INVALID_SOCKET) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }

    virtual ~srv_socket() {
        if (listen_socket != INVALID_SOCKET) closesocket(listen_socket);
        WSACleanup();
    }

    bool read_exact(SOCKET s, void* buffer, size_t size) {
        size_t total = 0;
        char* out = static_cast<char*>(buffer);
        while (total < size) {
            int received = recv(s, out + total, static_cast<int>(size - total), 0);
            if (received <= 0) return false;
            total += static_cast<size_t>(received);
        }
        return true;
    }

    void start_listing(const char* ip, const char* port) {
        addrinfo hints{ };
        addrinfo* result = nullptr;
        hints.ai_family = AF_INET; // ai ud
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(ip, port, &hints, &result) != 0) return;

        listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (listen_socket == INVALID_SOCKET) {
            freeaddrinfo(result);
            return;
        }

        BOOL opt = TRUE;
        setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        if (bind(listen_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
            closesocket(listen_socket);
            freeaddrinfo(result);
            return;
        }

        freeaddrinfo(result);
        if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) return;

        while (true) {
            SOCKET client = accept(listen_socket, nullptr, nullptr);
            if (client != INVALID_SOCKET) {
                this->on_client_connected(client);
                closesocket(client);
            }
        }
    }

    virtual void on_client_connected(SOCKET clientSocket) = 0;

private:
    SOCKET listen_socket;
};

#endif