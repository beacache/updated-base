#pragma once

#include <lstate.h>
#include <lualib.h>

#include <globals.h>

namespace module::core::environment
{
    struct c_miscellaneous
    {
        static int get_objects(lua_State* L);
        static int getexecutorname(lua_State* L);
        static int identifyexecutor(lua_State* L);
        void register_library(lua_State* L);
    };

    inline c_miscellaneous miscellaneous{ };
}