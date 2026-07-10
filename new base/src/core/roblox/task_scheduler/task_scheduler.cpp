#include "task_scheduler.h"
#include <globals.h>
#include <environment.h>
#include <yielder/yielder.h>

namespace module::rbx
{
    int hook_scheduler(lua_State * L)
    {
        if (!globals.execution_queue.empty())
        {
            execution.execute_src(globals.exploit_thread, globals.execution_queue.front());
            globals.execution_queue.erase(globals.execution_queue.begin());
        }
        yielder.run_yield();

        return 0;
    }

    void renderstepped(lua_State * L)
    {
        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);

        lua_pushstring(L, "RunService");
        lua_pcall(L, 2, 1, NULL);

        lua_getfield(L, -1, "RenderStepped");
        lua_getfield(L, -1, "Connect");
        lua_pushvalue(L, -2);

        lua_pushcclosure(L, hook_scheduler, nullptr, NULL);
        lua_pcall(L, 2, NULL, NULL);
        lua_pop(L, 2);
    }

    bool task_scheduler_t::initialize()
    {
        uintptr_t sc = module::rbx::script_context.get(globals.last_data_model);
        if (!sc)
            return false;

        lua_State* roblox_state = module::rbx::script_context.get_lua_state(sc);
        if (!roblox_state)
            return false;

        globals.exploit_thread = lua_newthread(roblox_state);
        if (!globals.exploit_thread)
            return false;

        task_scheduler_t::set_capabilities(globals.exploit_thread, 8, globals.max_capabilities);
        module::core::g_environment.setup(globals.exploit_thread);

        renderstepped(globals.exploit_thread);

        return true;
    }

    void task_scheduler_t::queue_execution(const std::string & script)
    {
        std::lock_guard<std::mutex> lock(globals.execution_mutex);
        globals.execution_queue.push_back(script);
    }

    void task_scheduler_t::set_capabilities(Proto * proto, uintptr_t * capabilities)
    {
        proto->userdata = capabilities;
        for (int i = 0; i < proto->sizep; ++i)
            task_scheduler_t::set_capabilities(proto->p[i], capabilities);
    }

    void task_scheduler_t::set_capabilities(lua_State * state, int level, uintptr_t capabilities)
    {
        if (!state || !state->userdata)
            return;

        state->userdata->identity = level;
        state->userdata->capabilities = capabilities;
    }
}
