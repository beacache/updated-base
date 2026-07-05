#pragma once

#include <ldo.h>
#include <string>
#include <lualib.h>
#include <lstate.h>
#include <lapi.h>
#include <Luau/Compiler.h>
#include <Luau/BytecodeUtils.h>
#include <Luau/BytecodeBuilder.h>

namespace module::rbx
{
    struct c_execution
    {
        void execute_src(lua_State* L, std::string src);
    };

    inline c_execution execution{ };
}