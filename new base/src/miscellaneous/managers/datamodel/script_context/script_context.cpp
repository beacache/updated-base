#include "script_context.h"
#include <globals.h>

namespace module::rbx
{
    uintptr_t c_script_context::get(uintptr_t dm)
    {
        if (!dm)
            return 0;

        uintptr_t children = module::update::instance::children.get(dm);
        if (!children)
            return 0;

        uintptr_t first = *reinterpret_cast<uintptr_t*>(children);
        if (!first)
            return 0;

        return module::update::data_model::script_context.get(first);
    }

    lua_State * c_script_context::get_lua_state(uintptr_t sc)
    {
        if (!sc)
            return nullptr;

        uint64_t null = 0;
        return module::update::luau::get_lua_state_for_instance(sc, &null, &null);
    }
}