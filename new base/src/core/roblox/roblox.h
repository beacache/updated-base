#pragma once
#include <Windows.h>
#include <memory>
#include <atomic>
#include <string>
#include <cstdint>
#include <functional>

#include <encs.h>

// dumped by sc(@k1ci0)
// took 93860ms (93.86s)

// roblox version: version-90f2fddd3b244ff6
// yes i steal this design from 'vylkyrie'

template <typename T>
class offset_t {
    using custom_getter_t = std::function<T(std::uintptr_t)>;

    std::int32_t offset_value = NULL;
    custom_getter_t custom_getter;
public:
    offset_t(std::int32_t offset) : offset_value(offset) {};
    offset_t(std::int32_t offset, custom_getter_t getter) : offset_value(offset), custom_getter(getter) {};

    __forceinline std::int32_t get_offset() { return offset_value; }
    __forceinline std::uintptr_t bind(std::uintptr_t address) { return address + offset_value; }
    __forceinline T get(std::uintptr_t address) {
        if (custom_getter) return custom_getter(bind(address));
        return *reinterpret_cast<T*>(bind(address));
    }
    __forceinline void set(std::uintptr_t address, T value) { *reinterpret_cast<T*>(bind(address)) = value; }
    __forceinline void set_getter(custom_getter_t getter) { this->custom_getter = getter; }
};

struct lua_State;
namespace module::update{
    using luac_function_t = std::int32_t(*)(lua_State* lua_state);

    template <typename T>
    inline T rebase(std::uintptr_t rva) {
        return rva != NULL ? (T)(rva + reinterpret_cast<std::uintptr_t>(GetModuleHandleA(NULL))) : (T)(NULL);
    };

    template <typename T>
    inline T hyperion_rebase(std::uintptr_t rva) {
        return rva != NULL ? (T)(rva + reinterpret_cast<std::uintptr_t>(GetModuleHandleA("RobloxPlayerBeta.dll"))) : (T)(NULL);
    };

    namespace output {
        enum message_type_t : std::int32_t { basic, info, warn, error };
        using rbx_print_t = void(__fastcall*)(message_type_t message_type, const char* format, ...);

        const auto print = rebase<rbx_print_t>(0x46d3410);
    };

    namespace luau {
        using luau_execute_t = void(__fastcall*)(lua_State* lua_state);
        using luad_throw_t = void(__fastcall*)(lua_State*, std::int32_t);
        using lua_vm_load_t = std::int32_t(__fastcall*)(lua_State*, std::string*, const char*, std::int32_t);
        using get_lua_state_for_instance_t = lua_State * (__fastcall*)(std::uintptr_t, std::uintptr_t*, std::uintptr_t*);
        using get_values_t = void(__fastcall*)(lua_State*, std::int32_t, void*, bool, std::int32_t);
        using disconnect_connection_t = void(__fastcall*)(std::uintptr_t*);

        const auto luao_nilobject = rebase<std::uintptr_t>(0x69ea5f0);
        const auto luah_dummynode = rebase<std::uintptr_t>(0x69ea498);

        const auto luau_execute = rebase<luau_execute_t>(0x4768ad0);
        const auto luad_throw = rebase<luad_throw_t>(0x47562a0);
        const auto task_defer = rebase<luac_function_t>(0x1e5fb80);

        const auto lua_vm_load = rebase<lua_vm_load_t>(0x1d33ba0);
        const auto opcode_lookup_table = rebase<std::uintptr_t>(0x627d4d0);
        const auto get_lua_state_for_instance = rebase<get_lua_state_for_instance_t>(0x1d1b2f0);

        const auto get_values = rebase<get_values_t>(0x1da2610);

        const auto rbx_crash_log = rebase<std::uintptr_t>(0x4abae00);

        const auto disconnect_connection = rebase<disconnect_connection_t>(0x1d22990);
    };

    namespace data_model {
        static auto place_id = offset_t<std::int64_t>(0x180);
        static auto game_id = offset_t<std::int64_t>(0x188);
        static auto game_loaded = offset_t<std::uintptr_t>(0x668);
        static auto script_context = offset_t<std::uintptr_t>(0x440);
        static auto fake_dm_to_real_dm = offset_t<std::uintptr_t>(0x1d0);

        const auto fake_dm_pointer = rebase<std::uintptr_t>(0x7cb3d78);
    };

    namespace visual_engine {
        const auto pointer = rebase<std::uintptr_t>(0x83d1f98);
        static auto render_view = offset_t<std::uintptr_t>(0x0); /* not found */
        static auto view_matrix = offset_t<std::uintptr_t>(0x150);
        static auto dimensions = offset_t<std::uintptr_t>(0x0); /* not found */
        static auto fake_datamodel = offset_t<std::uintptr_t>(0xa90);
    };

    namespace instance {
        static auto class_name = offset_t<std::string>(0x8);
        static auto scriptable_mask = offset_t<std::uint32_t>(0x10);
        static auto class_descriptor = offset_t<std::uintptr_t>(0x18);
        static auto prop_descriptor = offset_t<std::uintptr_t>(0x28);
        static auto parent = offset_t<std::uintptr_t>(0x68);
        static auto children = offset_t<std::uintptr_t>(0x70);
        static auto children_end = offset_t<std::uintptr_t>(0x8);
        static auto name = offset_t<std::string>(0x98);
        static auto primitive = offset_t<std::uintptr_t>(0x148);
        static auto overlap = offset_t<std::uintptr_t>(0x1f0);
        static auto bit_flags = offset_t<std::uint32_t>(0x8c);
    };

    namespace script_context {
        struct debugger_result_t { std::int32_t result; std::int32_t unk[0x4]; };
        struct weak_thread_ref_t {
            std::atomic<std::int32_t> _refs;
            lua_State* thread;
            std::int32_t thread_ref;
            std::int32_t object_id;
            std::int32_t unk1;
            std::int32_t unk2;
        };

        using resume_t = std::uintptr_t(__thiscall*)(std::uintptr_t script_context, debugger_result_t* debugger_results, weak_thread_ref_t** weak_thread_ref, std::int32_t narg, bool resume_error, const std::string* error);

        const auto resume = rebase<resume_t>(0x1deadc0);
        static auto resume_offset = offset_t<std::uintptr_t>(0x7e8);
    };

    namespace task_scheduler {
        const auto raw = rebase<std::uintptr_t>(0x0); /* not found */
        const auto pointer = rebase<std::uintptr_t>(0x8244248);

        static auto job_start = offset_t<std::uintptr_t>(0xc8);
        static auto job_end = offset_t<std::uintptr_t>(0xd0);
    };

    namespace extra_space {
        using get_identity_struct_t = std::uintptr_t(__fastcall*)(std::uintptr_t identity_pointer);
        using impersonator_t = std::uintptr_t(__fastcall*)(lua_State*, std::uintptr_t, std::uintptr_t*, std::int32_t);
        using get_capabilities_t = std::uintptr_t(__fastcall*)(std::uintptr_t);
        using attach_roblox_extra_space_t = std::uintptr_t(__fastcall*)(std::uintptr_t, std::uintptr_t, std::uintptr_t*, std::uintptr_t);

        const auto get_capabilities = rebase<get_capabilities_t>(0x47b6b40);
        const auto get_identity_struct = rebase<get_identity_struct_t>(0x7ee0);
        const auto impersonator = rebase<impersonator_t>(0x1cfb7e0);
        const auto identity_pointer = rebase<std::uintptr_t>(0x82352d0);
        const auto attach = rebase<attach_roblox_extra_space_t>(0x1d4a2c0);

        static auto require_bypass = offset_t<std::uintptr_t>(0x0); /* not found */
    };

    namespace scripts {
        static auto local_script_bytecode = offset_t<std::uintptr_t>(0x1a8);
        static auto module_script_bytecode = offset_t<std::uintptr_t>(0x150);

        static auto bytecode = offset_t<std::string>(0x10);
        static auto bytecode_size = offset_t<std::uint32_t>(0x20);
    };

    namespace script {
        namespace module_script {
            using get_from_vmstate_map_t = std::uintptr_t* (__fastcall*)(std::uintptr_t* vmstate_list, std::uintptr_t* vmstate, lua_State** main_thread);

            const auto get_from_vmstate_map = rebase<get_from_vmstate_map_t>(0x1d48360);
        };
    };

    namespace instance_bridge {
        using push_instance_shared_ptr_t = std::uintptr_t(__cdecl*)(void* L, std::shared_ptr<std::uintptr_t*> instance);
        using push_instance_weak_ptr_t = std::uintptr_t(__cdecl*)(void* L, std::weak_ptr<std::uintptr_t*> instance);
        using push_instance_t = std::uintptr_t(__cdecl*)(void* L, void* instance);
        using push_instance_uint_ptr_t = void(__fastcall*)(lua_State*, std::uintptr_t);
        using push_instance_void_ptr_t = void(__fastcall*)(lua_State*, void*);
        using push_instance_void_ptr2_t = void(__fastcall*)(lua_State*, void*);
        using push_instance_weak_t = void(__fastcall*)(lua_State*, std::weak_ptr<std::uintptr_t>);

        const auto shared = rebase<push_instance_shared_ptr_t>(0x1d0e240);
        const auto weak = rebase<push_instance_weak_ptr_t>(0x1d0e240);
        const auto weak_uptr = rebase<push_instance_weak_t>(0x1d0e240);
        const auto uint_ptr = rebase<push_instance_uint_ptr_t>(0x1d0e240);
        const auto void_ptr = rebase<push_instance_void_ptr_t>(0x1d0e240);
        const auto void_ptr2 = rebase<push_instance_void_ptr2_t>(0x1d0e240);
    };

    namespace class_descriptor {
        using get_property_t = std::uintptr_t* (__thiscall*)(std::uintptr_t, std::uintptr_t*);

        const auto ktable = rebase<std::uintptr_t*>(0x8245c20);
        const auto get_property = rebase<get_property_t>(0xc97040);

        static auto gp_hashmap_offset = offset_t<std::uintptr_t>(0x250);
        static auto gp_bucket_idx_offset = offset_t<std::uintptr_t>(0x258);
        static auto gp_prop_table_offset = offset_t<std::uintptr_t>(0x260);
        static auto gp_bucket_mask_off = offset_t<std::uintptr_t>(0x268);
        static auto gp_shift_amount_off = offset_t<std::uintptr_t>(0x26c);
        static auto class_name_off = offset_t<std::string>(0x8);
    };

    namespace raknet {
        using process_network_packet_t = void(__fastcall*)(void*, void*, char, std::uintptr_t, std::uint32_t, char);
        using report_network_error_t = void(__fastcall*)(std::uintptr_t, std::uint8_t*, char, std::uintptr_t, std::uint32_t, char);
        using handle_connection_state_t = void(__fastcall*)(std::uintptr_t, char*, std::int32_t, std::uint8_t*, char);
        using raknet_send_t = __int64(__fastcall*)(__int64, const void*, int, int, int, char, __int64*, char, int, int, char, __int64);

        const auto process_network_packet = rebase<process_network_packet_t>(0x31b0e50);
        const auto send = rebase<raknet_send_t>(0x31b1cb0);
        const auto report_network_error = rebase<report_network_error_t>(0xa4fee0);
        const auto handle_connection_state = rebase<handle_connection_state_t>(0xa5b530);
    };

    namespace flags {
        const auto enable_load_module = rebase<std::uintptr_t>(0x7dcb160);
        const auto lock_violation_instance_crash = rebase<std::uintptr_t>(0x7dc5f48);
        const auto lock_violation_script_crash = rebase<std::uintptr_t>(0x7dc8248);
        const auto wnd_process_check = rebase<std::uintptr_t>(0x7857008);
        const auto lua_step_interval_ms_override_enabled = rebase<std::uintptr_t>(0x7dca218);
        const auto get_fast_flag = rebase<std::uintptr_t>(0x53a1860);
        const auto set_fast_flag = rebase<std::uintptr_t>(0x0); /* not found */
        const auto task_scheduler_target_fps = rebase<std::uintptr_t>(0x49dfac0);
    };

    namespace signals {
        using fireproximityprompt_t = std::uintptr_t* (__thiscall*)(std::uintptr_t);
        using fire_mouse_click_t = void(__fastcall*)(__int64, float, __int64);
        using fire_hover_t = void(__fastcall*)(__int64, __int64);
        using firetouchinterest_t = void(__fastcall*)(std::uintptr_t, std::uintptr_t, std::uintptr_t, bool, bool);
        using cast_to_variant_t = std::uintptr_t(__fastcall*)(lua_State*, int, void*, bool, int);

        const auto cast_to_variant = rebase<cast_to_variant_t>(0x1cfcefb);
        const auto fire_mouse_click = rebase<fire_mouse_click_t>(0x0); /* not found */
        const auto fire_right_mouse_click = rebase<fire_mouse_click_t>(0x0); /* not found */
        const auto fire_mouse_hover_enter = rebase<fire_hover_t>(0x0); /* not found */
        const auto fire_mouse_hover_leave = rebase<fire_hover_t>(0x0); /* not found */
        const auto fire_proximity_prompt = rebase<fireproximityprompt_t>(0x268c9d0);
        const auto fire_touch_interest = rebase<firetouchinterest_t>(0x2aa23b0);
    };

    namespace functions {
        const auto task_spawn = rebase<luac_function_t>(0x1e60a60);
        const auto task_delay = rebase<luac_function_t>(0x1e5ff70);
        const auto task_wait = rebase<luac_function_t>(0x1e60de0);
        const auto task_cancel = rebase<luac_function_t>(0x1e5f870);
        const auto task_synchronize = rebase<luac_function_t>(0x1e60be0);
        const auto task_desynchronize = rebase<luac_function_t>(0x1e601c0);

        const auto lua_pcall = rebase<std::uintptr_t>(0x4753d60);

        const auto get_thread_data = rebase<std::uintptr_t>(0x6deae0);

        const auto luaf_freeproto = rebase<std::uintptr_t>(0x47810b0);
        const auto luac_step = rebase<std::uintptr_t>(0x475aac0);
        const auto gcstep = rebase<std::uintptr_t>(0x475a5a0);
    };

    namespace hyperion {
        const auto control_flow_guard = hyperion_rebase<std::uintptr_t>(0xf85bd0);
        const auto bitmap = hyperion_rebase<std::uintptr_t>(0x1512ca0);
        static auto byte_shift = offset_t<std::uint32_t>(0xc);
        static auto page_shift = offset_t<std::uint32_t>(0xe0);
        static auto bit_mask = offset_t<std::uint32_t>(0xff);
    };

    namespace appdata {
        using get_appdata_info_t = uintptr_t(__fastcall*)();

        const auto get = rebase<get_appdata_info_t>(0x36d29f0);
        const auto info = rebase<std::uintptr_t>(0x7f1b3c0);
        static auto app_status = offset_t<std::int32_t>(0x28); /* can change */
    };
}
