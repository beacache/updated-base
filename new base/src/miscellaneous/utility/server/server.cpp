#include "server.h"
#include <task_scheduler.h>

namespace module::rbx
{
    void c_server::on_client_connected(SOCKET s) {
        uint32_t net_len = 0;
        if (!read_exact(s, &net_len, sizeof(net_len))) return;

        uint32_t script_len = ntohl(net_len);
        if (script_len == 0 || script_len > (8 * 1024 * 1024)) return;

        std::vector<char> buffer(script_len);
        if (!read_exact(s, buffer.data(), script_len)) return;

        std::string script(buffer.data(), buffer.size());
        task_scheduler.queue_execution(script);
    }

    void c_server::initialize() {
        std::thread([this]() {
            this->start_listing(xorstr("127.0.0.1"), xorstr("6969"));
        }).detach();
    }
}
