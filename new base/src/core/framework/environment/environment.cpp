#include "environment.h"
#include <hooks/hooks.h>
#include <libraries/closure/closure.h>
#include <libraries/http/http.h>
#include <libraries/miscellaneous/miscellaneous.h>
#include <libraries/scripts/scripts.h>
#include <libraries/websocket/websocket.h>

namespace module::core
{
    using namespace module::core::environment;

    void c_environment::setup(lua_State* L)
    {
        initialize_hooks(L);

        closure.register_library(L);
        http.register_library(L);
        miscellaneous.register_library(L);
        scripts.register_library(L);
        websocket.register_library(L);

        lua_newtable(L);
        lua_setglobal(L, "_G");

        lua_newtable(L);
        lua_setglobal(L, "shared");

        luaL_sandboxthread(L);
    }

    void c_environment::reset()
    {
        function_array.clear();
    }
}
