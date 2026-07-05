#include "task_scheduler.h"
#include <globals.h>
#include <environment.h>
#include <yielder/yield.h>

namespace module::rbx
{
    bool g_task_scheduler_t::initialize()
    {
        uintptr_t sc = module::rbx::script_context.get(globals.last_data_model);
        lua_State* ls = module::rbx::script_context.get_lua_state(sc);
        lua_State* roblox_state = module::rbx::script_context.get_lua_state(sc);

        globals.exploit_thread = lua_newthread(roblox_state);
        g_task_scheduler_t::set_capabilities(globals.exploit_thread, 8, globals.max_capabilities);
        module::core::g_environment.setup(globals.exploit_thread);
        g_task_scheduler_t::setup_execution(globals.exploit_thread);

        return true;
    }

    static int hook_scheduler(lua_State* state)
    {
        {
            std::lock_guard<std::mutex> lock(globals.yield_mutex);
            while (!globals.yielder_queue.empty())
            {
                auto request = globals.yielder_queue.front();
                globals.yielder_queue.pop();
                try { request(); } catch (...) {}
            }
        }

        if (!globals.execution_queue.empty())
        {
            std::string src = globals.execution_queue.front();
            globals.execution_queue.erase(globals.execution_queue.begin());
            execution.execute_src(globals.exploit_thread, src);
        }

        //yielder::run();

        return 0;
    }

    void g_task_scheduler_t::setup_execution(lua_State* L)
    {
        lua_getglobal(L, "game");

        if (lua_type(L, -1) == LUA_TNIL)
        {
            lua_pop(L, 1);
            return;
        }

        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);
        lua_pushstring(L, "RunService");

        if (lua_pcall(L, 2, 1, NULL) != LUA_OK)
        {
            lua_pop(L, 2);
            return;
        }

        lua_getfield(L, -1, "Heartbeat");
        lua_getfield(L, -1, "Connect");

        lua_pushvalue(L, -2);
        lua_pushcclosure(L, hook_scheduler, nullptr, 0);

        if (lua_pcall(L, 2, 0, NULL) != LUA_OK)
        {
            lua_pop(L, 3);
            return;
        }

        lua_pop(L, 2);
    }

    void g_task_scheduler_t::queue_execution(const std::string& script)
    {
        globals.execution_queue.push_back(script);
    }

    void g_task_scheduler_t::set_capabilities(Proto* proto, uintptr_t* capabilities)
    {
        proto->userdata = capabilities;
        for (int i = 0; i < proto->sizep; ++i)
            g_task_scheduler_t::set_capabilities(proto->p[i], capabilities);
    }

    void g_task_scheduler_t::set_capabilities(lua_State* state, int level, uintptr_t capabilities)
    {
        state->userdata->identity = level;
        state->userdata->capabilities = capabilities;
    }
}