#include "tp_handler.h"
#include <globals.h>
#include <task_scheduler.h>
#include <server/server.h>

namespace module::rbx
{
    static bool read_app_status(int32_t & status)
    {
        uintptr_t info = module::update::appdata::get();
        if (!info)
            return false;

        status = module::update::appdata::app_status.get(info);
        return true;
    }

    static bool is_game_ready(uintptr_t dm)
    {
        if (!dm)
            return false;

        int32_t status = 0;
        if (!read_app_status(status))
            return false;

        if (status != 4)
            return false;

        return true;
    }

    void c_tp_handler::loop()
    {
        uintptr_t last_dm = 0;

        while (running)
        {
            Sleep(100);

            if (!running)
                break;

            uintptr_t dm = module::rbx::datamodel.get();

            if (dm != last_dm)
            {
                {
                    std::lock_guard<std::mutex> lock(globals.execution_mutex);
                    globals.execution_queue.clear();
                }

                {
                    std::lock_guard<std::mutex> lock(globals.yield_mutex);
                    while (!globals.yielder_queue.empty())
                        globals.yielder_queue.pop();
                }

                globals.exploit_thread = nullptr;
                cached_dm = 0;

                if (!dm)
                {
                    last_dm = 0;
                    continue;
                }

                if (!is_game_ready(dm))
                {
                    last_dm = dm;
                    continue;
                }

                uintptr_t sc = module::rbx::script_context.get(dm);
                if (!sc)
                {
                    last_dm = dm;
                    continue;
                }

                lua_State* ls = module::rbx::script_context.get_lua_state(sc);
                if (!ls || !ls->userdata)
                {
                    last_dm = dm;
                    continue;
                }

                globals.last_data_model = dm;
                cached_dm = dm;

                if (task_scheduler.initialize())
                {
                    task_scheduler.queue_execution("print(\"updated\")");
                }

                last_dm = dm;
            }
        }
    }

    void c_tp_handler::initialize()
    {
        if (running)
            return;

        running = true;
        worker = std::thread([this]()
        {
            loop();
        });
        worker.detach();
    }

    void c_tp_handler::stop()
    {
        running = false;
    }
}
