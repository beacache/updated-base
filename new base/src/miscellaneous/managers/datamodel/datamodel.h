#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <lstate.h>
#include <roblox.h>

namespace module::rbx
{
    struct c_datamodel
    {
        static uintptr_t get();
        static uintptr_t get_children_ptr(uintptr_t instance);
        static uintptr_t get_parent(uintptr_t instance);
        static std::string get_class_name(uintptr_t instance);
        static std::string get_name(uintptr_t instance);
        static uintptr_t find_first_child(uintptr_t instance, const std::string& name);
        static uintptr_t find_first_child_of_class(uintptr_t instance, const std::string& class_name);
        static std::vector<uintptr_t> get_children(uintptr_t instance);
        static uintptr_t get_local_player(uintptr_t dm);
    };

    inline c_datamodel datamodel{};
}