#include <core/core.h>
#include <tp_handler.h>
#include <server/server.h>

std::int32_t module::core::main_thread(HMODULE module)
{
    module::rbx::server.initialize();
    module::rbx::tp_handler.initialize();
    
    while (module::rbx::tp_handler.running)
        Sleep(250);

    return EXIT_SUCCESS;
}