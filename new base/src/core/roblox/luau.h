#pragma once
#include "roblox.h"

#pragma warning(disable: 4003)
#pragma warning(disable: 4172)

/*
              SET                                 GET
    vmvalue0: data = value                     || value = data
    vmvalue1: data = (value + (data + offset)) || value = (data - (value + offset))
    vmvalue2: data = ((data + offset) - value) || value = ((v + offset) - data)
    vmvalue3: data = (value ^ (data + offset)) || value = ((v + offset) ^ data)
    vmvalue4: data = (value - (data + offset)) || value = ((v + offset) + data)
*/

#define proto_lineinfo     vmvalue3/* proto_lineinfo_enc */
#define proto_debuginsn    vmvalue3/* proto_debuginsn_enc */
#define proto_typeinfo     vmvalue1/* proto_typeinfo_enc */
#define proto_abslineinfo  vmvalue2/* proto_abslineinfo_enc */
#define proto_source       vmvalue3/* proto_source_enc */
#define proto_locvars      vmvalue2/* proto_locvars_enc */
#define proto_upvalues     vmvalue4/* proto_upvalues_enc */
#define proto_debugname    vmvalue4/* proto_debugname_enc */
#define proto_userdata     vmvalue2/* proto_userdata_enc */
#define udata_meta         vmvalue3/* udata_meta_enc */
#define closure_debugname  vmvalue1/* closure_debugname_enc */
#define closure_cont       vmvalue2/* closure_cont_enc */
#define tstring_hash       vmvalue3/* tstring_hash_enc */
#define lstate_stacksize   vmvalue1/* lstate_stacksize_enc */
