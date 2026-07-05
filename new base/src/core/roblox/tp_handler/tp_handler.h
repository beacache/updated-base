#pragma once

#include <atomic>
#include <thread>
#include <Windows.h>
#include <lstate.h>

namespace module::rbx
{
    struct c_tp_handler
    {
        std::atomic<bool> running{ false };
        std::atomic<uintptr_t> cached_dm{ 0 };
        std::thread worker{ };

        void loop();
        void initialize();
        void stop();
    };

    inline c_tp_handler tp_handler{ };
}