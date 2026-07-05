#pragma once

#include <lstate.h>
#include <lualib.h>
#include <lgc.h>
#include <lmem.h>
#include <unordered_map>
#include <memory>

#include <globals.h>
#include <roblox.h>

#include <sol/sol.hpp>
#include <environment.h>
#include <ltable.h>

namespace module::core::environment
{
    struct instance_context
    {
        lua_State* L;
        int n;
    };

    struct gco_context_loaded
    {
        lua_State* p_lua;
        int items_found;
        std::unordered_map<uintptr_t, bool> map;
    };

    struct gco_context_running
    {
        lua_State* p_lua;
        int n;
        std::unordered_map<uintptr_t*, bool> running_scripts;
    };

    struct c_scripts
    {
        static int getgenv(lua_State* L);
        static int getscripts(lua_State* L);
        static int getloadedmodules(lua_State* L);
        static int getrunningscripts(lua_State* L);

        static bool getscripts_visit(void* ctx, lua_Page* page, GCObject* gco);
        static bool getloadedmodules_visit(void* ctx, lua_Page* page, GCObject* gco);
        static bool getrunningscripts_visit(void* ctx, lua_Page* page, GCObject* gco);

        void register_library(lua_State* L);
    };

    inline c_scripts scripts{};
}