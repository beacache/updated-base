#pragma once

#include <string>
#include <cstdint>
#include <lstate.h>
#include <globals.h>

namespace module::rbx
{
    struct g_task_scheduler_t
    {
        bool initialize();
        void queue_execution(const std::string& script);
        void set_capabilities(Proto* proto, uintptr_t* capabilities);
        void set_capabilities(lua_State* state, int level, uintptr_t capabilities);
        void setup_execution(lua_State* state);
    };

    inline g_task_scheduler_t g_task_scheduler{};
}