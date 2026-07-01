#include "jobs.h"
#include <globals.h>

namespace module::rbx
{
    uintptr_t c_jobs::get_task_scheduler()
    {
        uintptr_t ptr = module::update::task_scheduler::pointer;
        if (!ptr)
            return 0;

        uintptr_t scheduler = *reinterpret_cast<uintptr_t*>(ptr);
        if (!scheduler)
            return 0;

        return scheduler;
    }

    std::string c_jobs::get_job_name(uintptr_t job)
    {
        if (!job)
            return "";

        try
        {
            uintptr_t name_addr = job + 0x18;

            const char* inline_buf = reinterpret_cast<const char*>(name_addr);
            size_t len = *reinterpret_cast<size_t*>(name_addr + 0x10);

            if (len == 0 || len > 128)
                return "";

            const char* data = nullptr;

            if (len < 16)
                data = inline_buf;
            else
                data = *reinterpret_cast<const char**>(name_addr);

            if (!data)
                return "";

            for (size_t i = 0; i < len; i++)
            {
                if (data[i] < 0x20 || data[i] > 0x7E)
                    return "";
            }

            return std::string(data, len);
        }
        catch (...)
        {
            return "";
        }
    }

    std::vector<job_info> c_jobs::get_all_jobs()
    {
        std::vector<job_info> result;

        uintptr_t scheduler = get_task_scheduler();
        if (!scheduler)
            return result;

        uintptr_t start = *reinterpret_cast<uintptr_t*>(scheduler + 0xc8);
        uintptr_t end = *reinterpret_cast<uintptr_t*>(scheduler + 0xd0);

        if (!start || !end)
            return result;

        if (start >= end)
            return result;

        uintptr_t range = end - start;
        if (range > 0x800)
            return result;

        for (uintptr_t i = start; i < end; i += sizeof(uintptr_t))
        {
            uintptr_t job = *reinterpret_cast<uintptr_t*>(i);
            if (!job)
                continue;

            if (job < 0x10000)
                continue;

            job_info info;
            info.address = job;
            info.name = get_job_name(job);

            if (info.name.empty())
                continue;

            result.push_back(info);
        }

        return result;
    }

    uintptr_t c_jobs::find_job(const std::string & name)
    {
        auto all = get_all_jobs();

        for (const auto& job : all)
        {
            if (job.name == name)
                return job.address;
        }

        return 0;
    }

    uintptr_t c_jobs::get_script_context_from_job(uintptr_t job)
    {
        if (!job)
            return 0;

        uintptr_t sc = *reinterpret_cast<uintptr_t*>(job + 0x1b0);
        if (!sc)
            sc = *reinterpret_cast<uintptr_t*>(job + 0x1a8);
        if (!sc)
            sc = *reinterpret_cast<uintptr_t*>(job + 0x1b8);

        return sc;
    }

    uintptr_t c_jobs::find_waiting_hybrid_script_job()
    {
        auto all = get_all_jobs();

        for (const auto& job : all)
        {
            if (job.name.find("WaitingHybridScriptsJob") != std::string::npos)
            {
                uintptr_t sc = get_script_context_from_job(job.address);
                if (sc)
                    return job.address;
            }
        }

        for (const auto& job : all)
        {
            if (job.name.find("WaitingHybridScriptsJob") != std::string::npos)
                return job.address;
        }

        return 0;
    }

    void c_jobs::print_all_jobs()
    {
        auto all = get_all_jobs();

        module::update::output::print(
            module::update::output::info,
            "[jobs] total: %d",
            (int)all.size()
        );

        for (const auto& job : all)
        {
            module::update::output::print(
                module::update::output::info,
                "[jobs] %s @ 0x%llX",
                job.name.c_str(),
                job.address
            );
        }
    }
}