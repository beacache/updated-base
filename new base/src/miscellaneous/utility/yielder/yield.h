#pragma once

#include <thread>
#include <lstate.h>
#include <functional>

struct yield_state
{
    uintptr_t State; // 0x0
    uintptr_t TimeUsed; // 0x8
    uintptr_t Useless; // 0x10
};

struct yielding_lua_thread
{
    char Pad0[0x28];
    lua_State* L; // 0x28
};

using yielded = std::function<int(lua_State*)>;
namespace yielder
{
    int execution(lua_State* L, const std::function<yielded()>& closure);
    void run();
}