#include "miscellaneous.h"
#include <sol/sol.hpp>

namespace module::core::environment
{
    using namespace module::rbx;

    int c_miscellaneous::get_objects(lua_State * L)
    {
        sol::state_view lua(L);

        if (!lua_isuserdata(L, 1))
        {
            luaL_typeerror(L, 1, "userdata");
            return 0;
        }

        if (!lua_isstring(L, 2))
        {
            luaL_typeerror(L, 1, "string");
            return 0;
        }

        auto asset_id = sol::stack::get<std::string>(L, 2);

        lua_getglobal(L, "game");
        lua_getfield(L, -1, "GetService");
        lua_pushvalue(L, -2);
        lua_pushstring(L, "InsertService");
        lua_call(L, 2, 1);
        lua_remove(L, -2);

        lua_getfield(L, -1, "LoadLocalAsset");
        lua_pushvalue(L, -2);
        sol::stack::push(L, asset_id);
        lua_pcall(L, 2, 1, NULL);

        if (lua_isstring(L, -1))
        {
            auto err = sol::stack::get<std::string>(L, -1);
            luaL_error(L, err.c_str());
        }

        lua_createtable(L, 1, 0);
        lua_pushvalue(L, -2);
        lua_rawseti(L, -2, 1);

        lua_remove(L, -3);
        lua_remove(L, -2);

        return 1;
    }

    int c_miscellaneous::getexecutorname(lua_State * L)
    {
        return sol::stack::push(L, "base");
    }

    int c_miscellaneous::identifyexecutor(lua_State * L)
    {
        return sol::stack::multi_push(L, "base", "1.0");
    }

    void c_miscellaneous::register_library(lua_State * L)
    {
        utils.add_function(L, "getexecutorname", c_miscellaneous::getexecutorname);
        utils.add_function(L, "identifyexecutor", c_miscellaneous::identifyexecutor);
    }
}