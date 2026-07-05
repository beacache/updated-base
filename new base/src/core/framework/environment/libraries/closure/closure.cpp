#include "closure.h"
#include <sol/sol.hpp>

namespace module::core::environment
{
    using namespace module::rbx;

    inline int c_closure::loadstring(lua_State * L)
    {
        sol::state_view lua(L);

        if (!lua.stack_top() || !lua_isstring(L, 1))
        {
            luaL_typeerror(L, 1, "string");
            return 0;
        }

        auto src = sol::stack::get<std::string>(L, 1);
        auto cn = lua_isstring(L, 2)
            ? sol::stack::get<std::string>(L, 2)
            : std::string("base");

        std::string bc = globals.compile_src(src);

        if (luau_load(L, cn.c_str(), bc.c_str(), bc.size(), NULL) != LUA_OK)
        {
            auto err = sol::stack::get<std::string>(L, -1);
            lua_pop(L, 1);
            return sol::stack::multi_push(L, sol::nil, err);
        }

        const TValue* obj = luaA_toobject(L, -1);
        if (obj && ttisfunction(obj))
        {
            Closure* func = clvalue(obj);
            if (func && !func->isC && func->l.p)
                module::rbx::g_task_scheduler.set_capabilities(func->l.p, &globals.max_capabilities);
        }

        lua_setsafeenv(L, LUA_GLOBALSINDEX, false);
        return 1;
    }

    void c_closure::register_library(lua_State * L)
    {
        utils.add_function(L, "loadstring", c_closure::loadstring);
    }
}