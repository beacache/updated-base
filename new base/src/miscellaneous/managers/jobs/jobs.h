#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <globals.h>
#include <Windows.h>

namespace module::rbx
{
    struct job_info
    {
        uintptr_t address;
        std::string name;
    };

    struct c_jobs
    {
        static uintptr_t get_task_scheduler();
        static std::vector<job_info> get_all_jobs();
        static uintptr_t find_job(const std::string& name);
        static std::string get_job_name(uintptr_t job);
        static uintptr_t find_waiting_hybrid_script_job();
        static uintptr_t get_script_context_from_job(uintptr_t job);
        static void print_all_jobs();
    };

    inline c_jobs jobs{};
}