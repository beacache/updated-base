#include "yielder.h"
#include <globals.h>

namespace module::rbx
{
    void g_yielder_t::run_yield()
    {
        std::lock_guard<std::mutex> lock(globals.yield_mutex);
        if (!globals.yielder_queue.empty())
        {
            std::function<void()> request = globals.yielder_queue.front();
            globals.yielder_queue.pop();
            request();
        }
    }

    int g_yielder_t::yield_execution(lua_State * L, const std::function<yielded_t()>&closure)
    {
        lua_pushthread(L);
        int thread_ref = lua_ref(L, -1);
        lua_pop(L, 1);

        std::thread([=]
        {
            yielded_t resume_function = closure();

            std::lock_guard<std::mutex> lock(globals.yield_mutex);
            globals.yielder_queue.emplace([=]() -> void
            {
                YieldState yield_state{ 0 };

                YieldingLuaThread yielding_thread_obj{};
                yielding_thread_obj.L = L;
                YieldingLuaThread* yielding_thread_ptr = &yielding_thread_obj;

                auto sc = reinterpret_cast<uintptr_t>(L->userdata->shared->scriptContext);

                module::update::script_context::resume2(
                    sc + module::update::script_context::resume_offset.get_offset(),
                    &yield_state,
                    &yielding_thread_ptr,
                    resume_function(L),
                    NULL,
                    NULL
                );

                lua_unref(L, thread_ref);
            });
        }).detach();

        return lua_yield(L, 0);
    }
}
