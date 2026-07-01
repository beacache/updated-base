#pragma once

#include <execution.h>
#include <task_scheduler.h>
#include <environment.h>
#include <luau.h>
#include <roblox.h>
#include <tp_handler.h>

#include <sol/sol.hpp>

#include <json/json.h>
#include <zstd/zstd.h>
#include <zstd/xxhash.h>
#include <server/server.h>
#include <xorstr/xorstr.hpp>

#include <managers/jobs/jobs.h>

#include <managers/datamodel/datamodel.h>
#include <managers/datamodel/script_context/script_context.h>

#include <string>
#include <vector>
#include <deque>
#include <lstate.h>
#include <lapi.h>
#include <mutex>
#include <queue>

namespace module::rbx
{
    class bytecode_encoder : public Luau::BytecodeEncoder
    {
        inline void encode(uint32_t* data, size_t count) override
        {
            for (auto i = 0; i < count;)
            {
                uint8_t opcode = LUAU_INSN_OP(data[i]);
                const auto table = reinterpret_cast<BYTE*>(module::update::luau::opcode_lookup_table);
                uint8_t final_opcode = opcode * 227;
                final_opcode = table[final_opcode];

                data[i] = (final_opcode) | (data[i] & ~0xFF);
                i += Luau::getOpLength(static_cast<LuauOpcode>(opcode));
            }
        }
    };

    inline std::deque<std::string> registered_names{ };

    struct c_utils
    {
        void add_function(lua_State* L, const char* name, lua_CFunction function)
        {
            registered_names.emplace_back(name);
            const char* stable_name = registered_names.back().c_str();

            lua_pushcfunction(L, function, stable_name, 0);
            module::core::g_environment.function_array.push_back(*reinterpret_cast<Closure**>(index2addr(L, -1)));
            lua_setglobal(L, stable_name);
        }

        void add_table_function(lua_State* L, const char* name, lua_CFunction function)
        {
            registered_names.emplace_back(name);
            const char* stable_name = registered_names.back().c_str();

            lua_pushcfunction(L, function, stable_name, 0);
            module::core::g_environment.function_array.push_back(*reinterpret_cast<Closure**>(index2addr(L, -1)));
            lua_setfield(L, -2, stable_name);
        }
    };

    struct g_globals
    {
        lua_State* exploit_thread{ nullptr };
        lua_State* roblox_state{ nullptr };

        uintptr_t last_data_model{ 0 };

        std::vector<std::string> execution_queue{ };
        std::mutex execution_mutex{ };

        std::mutex yield_mutex;
        std::queue<std::function<void()>> yielder_queue;

        uintptr_t max_capabilities{ 0x200000000000003FLL | 0x3FFFFFFFFFFF00LL };

        std::string compile_src(std::string src)
        {
            auto encoder = bytecode_encoder();
            static const char* mutable_globals[] = { "Game", "Workspace", "game", "plugin", "script", "shared", "workspace", "_G", "_ENV", nullptr };

            Luau::CompileOptions opt;
            opt.debugLevel = 1;
            opt.optimizationLevel = 1;
            opt.mutableGlobals = mutable_globals;
            opt.vectorLib = "Vector3";
            opt.vectorCtor = "new";
            opt.vectorType = "Vector3";

            return Luau::compile(src, opt, {}, &encoder);
        }
    };

    inline g_globals globals{ };
    inline c_utils utils{ };
}