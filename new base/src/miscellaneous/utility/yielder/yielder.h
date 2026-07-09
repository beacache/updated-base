#pragma once

#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <lstate.h>
#include <roblox.h>

struct YieldState
{
    uintptr_t State; // 0x0
    uintptr_t TimeUsed; // 0x8
    uintptr_t Useless; // 0x10
};

struct YieldingLuaThread
{
    char Pad0[0x28];
    lua_State* L; // 0x28
};

namespace module::rbx
{
    using yielded_t = std::function<int(lua_State*)>;

    struct g_yielder_t
    {
        std::queue<std::function<void()>> queue;
        std::mutex mutex;

        void run_yield();
        int yield_execution(lua_State* L, const std::function<yielded_t()>& closure);
    };

    inline g_yielder_t yielder{};
}
