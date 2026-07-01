#pragma once

#include <cstdint>
#include <lstate.h>
#include <roblox.h>

namespace module::rbx
{
    struct c_script_context
    {
        static uintptr_t get(uintptr_t dm);
        static lua_State* get_lua_state(uintptr_t sc);
    };

    inline c_script_context script_context{};
}