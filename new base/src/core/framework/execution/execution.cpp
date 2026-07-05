#include "execution.h"
#include <globals.h>
#include <task_scheduler.h>

namespace module::rbx
{
    void c_execution::execute_src(lua_State * L, std::string src)
    {
        if (!L || src.empty())
            return;

        int top = lua_gettop(L);
        lua_State* thread = lua_newthread(L);
        lua_pop(L, 1);

        if (!thread)
            return;

        luaL_sandboxthread(thread);
        g_task_scheduler.set_capabilities(thread, 8, globals.max_capabilities);

        std::string bc = globals.compile_src(src);
        if (bc.empty())
            return;

        if (luau_load(thread, "", bc.c_str(), bc.length(), NULL) != LUA_OK)
        {
            std::string err = lua_tostring(thread, -1);
            update::output::print(update::output::message_type_t::error, "%s", err.c_str());
            lua_pop(thread, 1);
            return;
        }

        Closure* cl = clvalue(luaA_toobject(thread, -1));
        if (!cl)
            return;

        g_task_scheduler.set_capabilities(cl->l.p, &globals.max_capabilities);

        lua_getglobal(thread, "task");
        if (lua_type(thread, -1) == LUA_TNIL)
        {
            std::string err = lua_tostring(thread, -1);
            update::output::print(update::output::message_type_t::error, "%s", err.c_str());
            lua_pop(thread, 1);
            return;
        }

        lua_getfield(thread, -1, "defer");
        if (lua_type(thread, -1) == LUA_TNIL)
        {
            std::string err = lua_tostring(thread, -1);
            update::output::print(update::output::message_type_t::error, "%s", err.c_str());
            lua_pop(thread, 2);
            return;
        }

        lua_remove(thread, -2);
        lua_insert(thread, -2);

        if (lua_pcall(thread, 1, NULL, NULL) != LUA_OK)
        {
            std::string err = lua_tostring(thread, -1);
            update::output::print(update::output::message_type_t::error, "%s", err.c_str());
            lua_pop(thread, 1);
            return;
        }

        lua_settop(L, top);
    }
}