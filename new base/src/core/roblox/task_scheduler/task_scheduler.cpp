#include "task_scheduler.h"
#include <globals.h>
#include <environment.h>

namespace module::rbx
{
    using job_step_t = uint32_t(__fastcall*)(uintptr_t, uintptr_t);
    static job_step_t original_job_step = nullptr;
    static uintptr_t* shadow_vtable = nullptr;

    static uint32_t __fastcall hooked_job_step(uintptr_t job, uintptr_t stats)
    {
        if (globals.exploit_thread)
        {
            {
                std::lock_guard<std::mutex> lock(globals.yield_mutex);
                while (!globals.yielder_queue.empty())
                {
                    auto func = globals.yielder_queue.front();
                    globals.yielder_queue.pop();
                    func();
                }
            }

            if (!globals.execution_queue.empty())
            {
                std::string src;
                {
                    std::lock_guard<std::mutex> lock(globals.execution_mutex);
                    if (!globals.execution_queue.empty())
                    {
                        src = globals.execution_queue.front();
                        globals.execution_queue.erase(globals.execution_queue.begin());
                    }
                }

                if (!src.empty())
                    execution.execute_src(globals.exploit_thread, src);
            }
        }

        return original_job_step(job, stats);
    }

    static bool safe_compare_job_name(uintptr_t job, const char* target)
    {
        __try
        {
            size_t length = *reinterpret_cast<size_t*>(job + 0x18 + 16);
            char* ptr = *reinterpret_cast<char**>(job + 0x18);
            char* name = (length < 16) ? reinterpret_cast<char*>(job + 0x18) : ptr;

            if (!name)
                return false;

            size_t target_len = strlen(target);
            if (length != target_len)
                return false;

            return memcmp(name, target, target_len) == 0;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static uintptr_t find_job(const char* name)
    {
        __try
        {
            uintptr_t scheduler = *reinterpret_cast<uintptr_t*>(module::update::task_scheduler::pointer);
            if (!scheduler)
                return 0;

            uintptr_t start = *reinterpret_cast<uintptr_t*>(scheduler + module::update::task_scheduler::job_start.get_offset());
            uintptr_t end = *reinterpret_cast<uintptr_t*>(scheduler + module::update::task_scheduler::job_end.get_offset());

            if (!start || !end)
                return 0;

            size_t count = (end - start) / sizeof(uintptr_t);

            for (size_t i = 0; i < count; i++)
            {
                uintptr_t job = *reinterpret_cast<uintptr_t*>(start + i * sizeof(uintptr_t));
                if (!job)
                    continue;

                if (safe_compare_job_name(job, name))
                    return job;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return 0;
        }

        return 0;
    }

    static bool setup_job_hook()
    {
        __try
        {
            uintptr_t job = find_job("WaitingHybridScriptsJob");

            if (!job)
            {
                job = find_job("Heartbeat");
            }

            if (!job)
            {
                return false;
            }

            uintptr_t* orig_vtable = *reinterpret_cast<uintptr_t**>(job);
            if (!orig_vtable)
            {
                return false;
            }

            shadow_vtable = new uintptr_t[32];
            memcpy(shadow_vtable, orig_vtable, 32 * sizeof(uintptr_t));

            original_job_step = reinterpret_cast<job_step_t>(orig_vtable[2]);
            shadow_vtable[2] = reinterpret_cast<uintptr_t>(hooked_job_step);

            DWORD old_protect;
            if (!VirtualProtect(reinterpret_cast<void*>(job), sizeof(uintptr_t), PAGE_READWRITE, &old_protect))
            {
                delete[] shadow_vtable;
                shadow_vtable = nullptr;
                return false;
            }

            *reinterpret_cast<uintptr_t**>(job) = shadow_vtable;
            VirtualProtect(reinterpret_cast<void*>(job), sizeof(uintptr_t), old_protect, &old_protect);

            module::update::output::print(
                module::update::output::message_type_t::warn,
                "job hooked"
            );

            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    bool g_task_scheduler_t::initialize()
    {
        uintptr_t sc = module::rbx::script_context.get(globals.last_data_model);
        if (!sc)
        {
            return false;
        }

        lua_State* roblox_state = module::rbx::script_context.get_lua_state(sc);
        if (!roblox_state)
        {
            return false;
        }

        globals.exploit_thread = lua_newthread(roblox_state);
        if (!globals.exploit_thread)
        {
            return false;
        }

        module::core::g_environment.setup(globals.exploit_thread);

        if (!original_job_step)
        {
            if (!setup_job_hook())
            {
                module::update::output::print(
                    module::update::output::message_type_t::error,
                    "job hook in initialize failed"
                );
                return false;
            }
        }

        return true;
    }

    void g_task_scheduler_t::queue_execution(const std::string& script)
    {
        std::lock_guard<std::mutex> lock(globals.execution_mutex);
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
        if (!state || !state->userdata)
            return;

        uintptr_t ud = (uintptr_t)state->userdata;
        *reinterpret_cast<uint64_t*>(ud + 0x68) = capabilities;
        *reinterpret_cast<int32_t*>(ud + 0x88) = level;
    }
}
