#pragma once
#include <sol/sol.hpp>
#include <lstate.h>
#include <lualib.h>
#include <globals.h>

namespace module::core::environment
{
    struct c_closure
    {
        static int loadstring(lua_State* L);
        void register_library(lua_State* L);
    };

    inline c_closure closure{ };
}