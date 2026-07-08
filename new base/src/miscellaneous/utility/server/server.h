#pragma once
#include <server/include/server.h> 
#include <globals.h>

namespace module::rbx
{
    class c_server : public srv_socket
    {
    public:
        void initialize();
        void on_client_connected(SOCKET s) override;
    };

    inline c_server server{ };
}
