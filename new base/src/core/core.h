#pragma once

#include <Windows.h>
#include <cstdint>
#include <thread>
#include <filesystem>

#include <lualib.h>
#include <sol/include/sol/state.hpp>

#include <roblox.h>
#include <globals.h>
#include <task_scheduler.h>
#include <execution.h>

#include <core/framework/environment/environment.h>

namespace module::core
{
    std::int32_t main_thread(HMODULE module);
}

BOOL APIENTRY DllMain(HMODULE dll_module, DWORD call_reason, LPVOID reserved)
{
    if (call_reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(dll_module);

        std::thread(
            [dll_module]()
            {
                module::core::main_thread(dll_module);
            }
        ).detach();
    }

    return TRUE;
}