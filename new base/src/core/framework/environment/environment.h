#pragma once
#include <string>
#include <vector>
#include <lstate.h>
#include <lualib.h>

namespace module::core
{
    struct c_environment
    {
        std::vector<Closure*> function_array;

        void setup(lua_State* L);
        void reset();
    };

    inline c_environment g_environment{ };
}