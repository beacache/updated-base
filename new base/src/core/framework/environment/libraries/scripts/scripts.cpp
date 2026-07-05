#include "scripts.h"

namespace module::core::environment
{
    using namespace module::rbx;

    int c_scripts::getgenv(lua_State* L)
    {
        sol::state_view lua(L);

        if (globals.exploit_thread == L)
        {
            lua_pushvalue(L, LUA_GLOBALSINDEX);
            return 1;
        }

        lua_rawcheckstack(L, 1);
        luaC_threadbarrier(L);
        luaC_threadbarrier(globals.exploit_thread);
        lua_pushvalue(globals.exploit_thread, LUA_GLOBALSINDEX);
        lua_xmove(globals.exploit_thread, L, 1);

        return 1;
    }

    bool c_scripts::getscripts_visit(void* ctx, lua_Page* page, GCObject* gco)
    {
        auto g_ctx = static_cast<instance_context*>(ctx);
        const auto type = gco->gch.tt;

        if (isdead(g_ctx->L->global, gco))
            return false;

        if (type != LUA_TUSERDATA)
            return false;

        TValue* top = g_ctx->L->top;
        top->value.p = reinterpret_cast<void*>(gco);
        top->tt = type;
        g_ctx->L->top++;

        if (strcmp(luaL_typename(g_ctx->L, -1), "Instance") != 0)
        {
            lua_pop(g_ctx->L, 1);
            return false;
        }

        lua_getfield(g_ctx->L, -1, "ClassName");
        const char* inst_class = lua_tolstring(g_ctx->L, -1, 0);
        lua_pop(g_ctx->L, 1);

        if (!inst_class)
        {
            lua_pop(g_ctx->L, 1);
            return false;
        }

        bool should_include = false;

        if (strcmp(inst_class, "LocalScript") == 0 || strcmp(inst_class, "ModuleScript") == 0)
        {
            should_include = true;
        }
        else if (strcmp(inst_class, "Script") == 0)
        {
            lua_getfield(g_ctx->L, -1, "RunContext");
            if (lua_type(g_ctx->L, -1) != LUA_TNIL)
            {
                if (lua_isnumber(g_ctx->L, -1))
                    should_include = (lua_tointeger(g_ctx->L, -1) == 2);
                else
                {
                    lua_getfield(g_ctx->L, -1, "Value");
                    if (lua_isnumber(g_ctx->L, -1))
                        should_include = (lua_tointeger(g_ctx->L, -1) == 2);
                    lua_pop(g_ctx->L, 1);
                }
            }
            lua_pop(g_ctx->L, 1);
        }

        if (should_include)
        {
            g_ctx->n++;
            lua_rawseti(g_ctx->L, -2, g_ctx->n);
        }
        else
        {
            lua_pop(g_ctx->L, 1);
        }

        return false;
    }

    int c_scripts::getscripts(lua_State* L)
    {
        sol::state_view lua(L);

        if (lua_gettop(L) > 0)
            lua_settop(L, 0);

        instance_context context = { L, 0 };
        lua_createtable(L, 0, 0);

        const auto old_threshold = L->global->GCthreshold;
        L->global->GCthreshold = SIZE_MAX;
        luaM_visitgco(L, &context, getscripts_visit);
        L->global->GCthreshold = old_threshold;

        return 1;
    }

    bool c_scripts::getloadedmodules_visit(void* ctx, lua_Page* page, GCObject* gco)
    {
        const auto p_ctx = static_cast<gco_context_loaded*>(ctx);
        const auto ctx_l = p_ctx->p_lua;

        if (isdead(ctx_l->global, gco))
            return false;

        if (gco->gch.tt != LUA_TFUNCTION)
            return false;

        ctx_l->top->value.gc = gco;
        ctx_l->top->tt = LUA_TFUNCTION;
        ctx_l->top++;

        lua_getfenv(ctx_l, -1);

        if (lua_isnil(ctx_l, -1))
        {
            lua_pop(ctx_l, 2);
            return false;
        }

        lua_getfield(ctx_l, -1, "script");

        if (lua_isnil(ctx_l, -1))
        {
            lua_pop(ctx_l, 3);
            return false;
        }

        if (strcmp(luaL_typename(ctx_l, -1), "Instance") == 0)
        {
            uintptr_t script_addr = *reinterpret_cast<uintptr_t*>(lua_touserdata(ctx_l, -1));

            lua_getfield(ctx_l, -1, "ClassName");
            const char* class_name = lua_tostring(ctx_l, -1);
            lua_pop(ctx_l, 1);

            if (class_name && strcmp(class_name, "ModuleScript") == 0
                && p_ctx->map.find(script_addr) == p_ctx->map.end())
            {
                p_ctx->map.insert({ script_addr, true });
                lua_rawseti(ctx_l, -4, ++p_ctx->items_found);
            }
            else
            {
                lua_pop(ctx_l, 1);
            }
        }
        else
        {
            lua_pop(ctx_l, 1);
        }

        lua_pop(ctx_l, 2);
        return false;
    }

    int c_scripts::getloadedmodules(lua_State* L)
    {
        sol::state_view lua(L);

        if (lua_gettop(L) > 0)
            lua_settop(L, 0);
        lua_newtable(L);

        gco_context_loaded gc_ctx = { L, 0 };

        const auto old_threshold = L->global->GCthreshold;
        L->global->GCthreshold = SIZE_MAX;
        luaM_visitgco(L, &gc_ctx, getloadedmodules_visit);
        L->global->GCthreshold = old_threshold;

        return 1;
    }

    bool c_scripts::getrunningscripts_visit(void* ctx, lua_Page* page, GCObject* gco)
    {
        const auto g_ctx = static_cast<gco_context_running*>(ctx);
        lua_State* L = g_ctx->p_lua;

        if (isdead(L->global, gco))
            return false;

        if (gco->gch.tt != LUA_TTHREAD)
            return false;

        lua_State* thread = reinterpret_cast<lua_State*>(gco);
        if (!thread->userdata)
            return false;

        const auto weak_ptr = reinterpret_cast<std::weak_ptr<uintptr_t>*>(
            reinterpret_cast<uintptr_t>(thread->userdata) + 0x48
        );

        if (!weak_ptr || weak_ptr->expired())
            return false;

        auto locked = weak_ptr->lock();
        if (!locked || thread->global->mainthread != L->global->mainthread)
            return false;

        if (g_ctx->running_scripts.emplace(locked.get(), true).second)
        {
            module::update::instance_bridge::weak_uptr(L, *weak_ptr);
            g_ctx->n++;
            lua_rawseti(L, -2, g_ctx->n);
        }

        return false;
    }

    int c_scripts::getrunningscripts(lua_State* L)
    {
        sol::state_view lua(L);

        lua_newtable(L);

        int table_index = lua_gettop(L);
        int i = 0;
        std::unordered_map<std::shared_ptr<std::uintptr_t*>, bool> running_scripts;

        lua_rawcheckstack(L, 5);
        luaC_threadbarrier(L);
        lua_pushvalue(L, LUA_REGISTRYINDEX);
        lua_pushnil(L);

        while (lua_next(L, -2))
        {
            if (lua_isthread(L, -1))
            {
                lua_State* thread = lua_tothread(L, -1);
                auto* data = static_cast<RobloxExtraSpace*>(thread->userdata);

                if (thread && data && !data->source.expired())
                {
                    if (thread->global->mainthread == L->global->mainthread)
                    {
                        const auto script = *reinterpret_cast<std::shared_ptr<std::uintptr_t*>*>(&data->source);

                        if (!running_scripts.contains(script))
                        {
                            module::update::instance_bridge::weak_uptr(L, data->source);

                            lua_getfield(L, -1, "ClassName");
                            const char* class_name = lua_tostring(L, -1);
                            lua_pop(L, 1);

                            bool is_valid = false;

                            if (class_name)
                            {
                                if (strcmp(class_name, "LocalScript") == 0 || strcmp(class_name, "ModuleScript") == 0)
                                    is_valid = true;
                                else if (strcmp(class_name, "Script") == 0)
                                {
                                    lua_getfield(L, -1, "RunContext");
                                    if (lua_type(L, -1) != LUA_TNIL)
                                    {
                                        if (lua_isnumber(L, -1))
                                            is_valid = (lua_tointeger(L, -1) == 2);
                                        else
                                        {
                                            lua_getfield(L, -1, "Value");
                                            if (lua_isnumber(L, -1))
                                                is_valid = (lua_tointeger(L, -1) == 2);
                                            lua_pop(L, 1);
                                        }
                                    }
                                    lua_pop(L, 1);
                                }
                            }

                            if (is_valid)
                            {
                                running_scripts[script] = true;
                                lua_rawseti(L, table_index, ++i);
                            }
                            else
                            {
                                lua_pop(L, 1);
                            }
                        }
                    }
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        return 1;
    }

    void c_scripts::register_library(lua_State* L)
    {
        utils.add_function(L, "getgenv",            c_scripts::getgenv);
        utils.add_function(L, "getscripts",         c_scripts::getscripts);
        utils.add_function(L, "getloadedmodules",   c_scripts::getloadedmodules);
        utils.add_function(L, "getrunningscripts",  c_scripts::getrunningscripts);
    }
}