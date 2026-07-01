#include "datamodel.h"
#include <globals.h>

namespace module::rbx
{
    uintptr_t c_datamodel::get()
    {
        uintptr_t fake_dm = *reinterpret_cast<uintptr_t*>(module::update::data_model::fake_dm_pointer);
        if (!fake_dm)
            return 0;

        return *reinterpret_cast<uintptr_t*>(
            module::update::data_model::fake_dm_to_real_dm.bind(fake_dm)
        );
    }

    uintptr_t c_datamodel::get_children_ptr(uintptr_t instance)
    {
        if (!instance)
            return 0;

        return module::update::instance::children.get(instance);
    }

    uintptr_t c_datamodel::get_parent(uintptr_t instance)
    {
        if (!instance)
            return 0;

        return module::update::instance::parent.get(instance);
    }

    std::string c_datamodel::get_class_name(uintptr_t instance)
    {
        if (!instance)
            return "";

        uintptr_t descriptor = module::update::instance::class_descriptor.get(instance);
        if (!descriptor)
            return "";

        return module::update::instance::class_name.get(descriptor);
    }

    std::string c_datamodel::get_name(uintptr_t instance)
    {
        if (!instance)
            return "";

        return *reinterpret_cast<std::string*>(instance + 0xb0);
    }

    uintptr_t c_datamodel::find_first_child(uintptr_t instance, const std::string & name)
    {
        if (!instance)
            return 0;

        auto children = get_children(instance);
        for (auto child : children)
        {
            if (get_name(child) == name)
                return child;
        }

        return 0;
    }

    uintptr_t c_datamodel::find_first_child_of_class(uintptr_t instance, const std::string & class_name)
    {
        if (!instance)
            return 0;

        auto children = get_children(instance);
        for (auto child : children)
        {
            if (get_class_name(child) == class_name)
                return child;
        }

        return 0;
    }

    std::vector<uintptr_t> c_datamodel::get_children(uintptr_t instance)
    {
        std::vector<uintptr_t> result;

        if (!instance)
            return result;

        uintptr_t children_ptr = get_children_ptr(instance);
        if (!children_ptr)
            return result;

        uintptr_t start = *reinterpret_cast<uintptr_t*>(children_ptr);
        uintptr_t end = *reinterpret_cast<uintptr_t*>(children_ptr + sizeof(uintptr_t));

        if (!start || !end || start >= end)
            return result;

        for (uintptr_t i = start; i < end; i += sizeof(uintptr_t))
        {
            uintptr_t child = *reinterpret_cast<uintptr_t*>(i);
            if (child)
                result.push_back(child);
        }

        return result;
    }

    uintptr_t c_datamodel::get_local_player(uintptr_t dm)
    {
        if (!dm)
            return 0;

        uintptr_t players = find_first_child_of_class(dm, "Players");
        if (!players)
            return 0;

        auto children = get_children(players);
        for (auto child : children)
        {
            if (get_class_name(child) == "Player")
                return child;
        }

        return 0;
    }
}