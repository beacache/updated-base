// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lobject.h"
#include "ltm.h"
#include "ludata.h"

typedef struct LuaNode LuaNode;

// registry
#define registry(L) (&L->global->registry)

// extra stack space to handle TM calls and some other extras
#define EXTRA_STACK 5

#define BASIC_CI_SIZE 8

#define BASIC_STACK_SIZE (2 * LUA_MINSTACK)

// clang-format off
struct stringtable
{
    struct TString** hash; /* offset 0 */
    int nuse; /* offset 8 */
    int size; /* offset 12 */
};
// clang-format on

/*
** informations about a call
**
** the general Lua stack frame structure is as follows:
** - each function gets a stack frame, with function "registers" being stack slots on the frame
** - function arguments are associated with registers 0+
** - function locals and temporaries follow after; usually locals are a consecutive block per scope, and temporaries are allocated after this, but
*this is up to the compiler
**
** when function doesn't have varargs, the stack layout is as follows:
** ^ (func) ^^ [fixed args] [locals + temporaries]
** where ^ is the 'func' pointer in CallInfo struct, and ^^ is the 'base' pointer (which is what registers are relative to)
**
** when function *does* have varargs, the stack layout is more complex - the runtime has to copy the fixed arguments so that the 0+ addressing still
*works as follows:
** ^ (func) [fixed args] [varargs] ^^ [fixed args] [locals + temporaries]
**
** computing the sizes of these individual blocks works as follows:
** - the number of fixed args is always matching the `numparams` in a function's Proto object; runtime adds `nil` during the call execution as
*necessary
** - the number of variadic args can be computed by evaluating (ci->base - ci->func - 1 - numparams)
**
** the CallInfo structures are allocated as an array, with each subsequent call being *appended* to this array (so if f calls g, CallInfo for g
*immediately follows CallInfo for f)
** the `nresults` field in CallInfo is set by the caller to tell the function how many arguments the caller is expecting on the stack after the
*function returns
** the `flags` field in CallInfo contains internal execution flags that are important for pcall/etc, see LUA_CALLINFO_*
*/
// clang-format off
struct CallInfo
{
    TValue* base; /* offset 0 */
    TValue* func; /* offset 8 */
    TValue* top; /* offset 16 */
    Proto* p; /* offset 24 */
    union /* offset 32 */
    {
        const Instruction* savedpc; /* offset 0 */
        int errfunc; /* offset 0 */
    };
    int nresults; /* offset 40 */
    unsigned int flags; /* offset 44 */
};
// clang-format on

#define LUA_CALLINFO_RETURN (1 << 0) // should the interpreter return after returning from this callinfo? first frame must have this set
#define LUA_CALLINFO_HANDLE (1 << 1) // should the error thrown during execution get handled by continuation from this callinfo? func must be C
#define LUA_CALLINFO_NATIVE (1 << 2) // should this function be executed using execution callback for native code
#define LUA_CALLINFO_OPYIELD (1 << 3) // call frame has yielded on a non-call opcode and requires luaV_finishop

#define curr_func(L) (clvalue(L->ci->func))
#define ci_func(ci) (clvalue((ci)->func))
#define f_isLua(ci) (!ci_func(ci)->isC)
#define isLua(ci) (ttisfunction((ci)->func) && f_isLua(ci))

struct GCStats
{
    // data for proportional-integral controller of heap trigger value
    int32_t triggerterms[32] = {0};
    uint32_t triggertermpos = 0;
    int32_t triggerintegral = 0;

    size_t atomicstarttotalsizebytes = 0;
    size_t endtotalsizebytes = 0;
    size_t heapgoalsizebytes = 0;

    double starttimestamp = 0;
    double atomicstarttimestamp = 0;
    double endtimestamp = 0;
};

struct GCCycleMetrics
{
    size_t starttotalsizebytes = 0;
    size_t heaptriggersizebytes = 0;

    double pausetime = 0.0; // time from end of the last cycle to the start of a new one

    double starttimestamp = 0.0;
    double endtimestamp = 0.0;

    double marktime = 0.0;
    double markassisttime = 0.0;
    double markmaxexplicittime = 0.0;
    size_t markexplicitsteps = 0;
    size_t markwork = 0;

    double atomicstarttimestamp = 0.0;
    size_t atomicstarttotalsizebytes = 0;
    double atomictime = 0.0;

    // specific atomic stage parts
    double atomictimeupval = 0.0;
    double atomictimeweak = 0.0;
    double atomictimegray = 0.0;
    double atomictimeclear = 0.0;

    double sweeptime = 0.0;
    double sweepassisttime = 0.0;
    double sweepmaxexplicittime = 0.0;
    size_t sweepexplicitsteps = 0;
    size_t sweepwork = 0;

    size_t assistwork = 0;
    size_t explicitwork = 0;

    size_t propagatework = 0;
    size_t propagateagainwork = 0;

    size_t endtotalsizebytes = 0;
};

struct GCMetrics
{
    double stepexplicittimeacc = 0.0;
    double stepassisttimeacc = 0.0;

    // when cycle is completed, last cycle values are updated
    uint64_t completedcycles = 0;

    GCCycleMetrics lastcycle;
    GCCycleMetrics currcycle;
};
// Callbacks that can be used to to redirect code execution from Luau bytecode VM to a custom implementation (AoT/JiT/sandboxing/...)
struct lua_ExecutionCallbacks
{
    void* context;
    void (*close)(lua_State* L);                 // called when global VM state is closed
    void (*destroy)(lua_State* L, Proto* proto); // called when function is destroyed
    int (*enter)(lua_State* L, Proto* proto);    // called when function is about to start/resume (when execdata is present), return 0 to exit VM
    void (*disable)(lua_State* L, Proto* proto); // called when function has to be switched from native to bytecode in the debugger
    size_t (*getmemorysize)(lua_State* L, Proto* proto); // called to request the size of memory associated with native part of the Proto
    uint8_t (*gettypemapping)(lua_State* L, const char* str, size_t len); // called to get the userdata type index
    char* (*getcounterdata)(
        lua_State* L,
        Proto* proto,
        size_t* count
    ); // called to get the execution counter data and count {uint32_t, uint32_t, uint64_t}
    Proto* (*inlinefunction)(lua_State* L, Closure* caller, Closure* target, uint32_t pc); // called when inlining threshold is reached
};

struct lua_UdataDirectAccessData
{
    TValue indextm;
    TValue newindextm;
    TValue namecalltm;
    lua_UserdataDirectAccess index;
    lua_UserdataDirectAccess newindex;
    lua_UserdataDirectNamecall namecall;
};

/*
** `global state', shared by all threads of this state
*/
// clang-format off
struct global_State
{
    struct stringtable strt; /* offset 0 */
    lua_Alloc frealloc; /* offset 16 */
    void* ud; /* offset 24 */
    int gcstepsize; /* offset 32 */
    int gcstepmul; /* offset 36 */
    int gcgoal; /* offset 40 */
    unsigned char gap_00[0x4]; /* offset 44 */
    size_t GCthreshold; /* offset 48 */
    size_t totalbytes; /* offset 56 */
    unsigned char currentwhite; /* offset 64 */
    unsigned char gcstate; /* offset 65 */
    unsigned char gap_01[0x6]; /* offset 66 */
    GCObject* weak; /* offset 72 */
    GCObject* grayagain; /* offset 80 */
    GCObject* gray; /* offset 88 */
    lua_Page* sweepgcopage; /* offset 96 */
    lua_Page* freegcopages[40]; /* offset 104 */
    UpVal uvhead; /* offset 424 */
    lua_Page* freepages[40]; /* offset 464 */
    lua_State* mainthread; /* offset 784 */
    lua_Page* allgcopages; /* offset 792 */
    struct lua_Page* allpages;
    LuaTable* mt[14]; /* offset 808 */
    TString* tmname[21]; /* offset 920 */
    TString* ttname[14]; /* offset 1088 */
    TValue pseudotemp; /* offset 1200 */
    TValue registry; /* offset 1216 */
    int registryfree; /* offset 1232 */
    struct lua_jmpbuf* errorjmp;
    unsigned __int64 rngstate; /* offset 1248 */
    unsigned __int64 ptrenckey[4]; /* offset 1256 */
    lua_Callbacks cb; /* offset 1288 */
    lua_ExecutionCallbacks ecb; /* offset 1368 */
    unsigned char ecbdata[512]; /* offset 1440 */
    lua_UdataDirectAccessData udatadirect[130]; /* offset 1952 */
    size_t memcatbytes[256]; /* offset 11312 */
    void (*udatagc[128])(struct lua_State*, void*); /* offset 13360 */
    LuaTable* udatamt[128]; /* offset 14384 */
    TString* lightuserdataname[128]; /* offset 15408 */
    struct LuaTable* udatadirectfields[130]; /* offset 16432 */
    struct GCStats gcstats; /* offset 17472 */
    unsigned int lastprotoid; /* offset 17656 */
#ifdef LUAI_GCMETRICS
    GCMetrics gcmetrics; /* offset 17664 */
#endif
};
// clang-format on

/*
** `per thread' state
*/
// clang-format off
struct lua_State
{
    CommonHeader; /* offset 0 */
    unsigned char status; /* offset 3 */
    unsigned char activememcat; /* offset 4 */
    bool singlestep; /* offset 5 */
    bool isactive; /* offset 6 */
    TValue* stack; /* offset 8 */
    global_State* global; /* offset 16 */
    TValue* stack_last; /* offset 24 */
    CallInfo* ci; /* offset 32 */
    TValue* base; /* offset 40 */
    TValue* top; /* offset 48 */
    struct RobloxExtraSpace* userdata; /* offset 56 */
    LuaTable* gt; /* offset 64 */
    lstate_stacksize<int> stacksize; /* offset 72 */
    int size_ci; /* offset 76 */
    UpVal* openupval;       // list of open upvalues in this stack
    CallInfo* end_ci; /* offset 88 */
    CallInfo* base_ci; /* offset 96 */
    GCObject* gclist; /* offset 104 */
    TString* namecall; /* offset 112 */
    unsigned short nCcalls; /* offset 120 */
    unsigned short baseCcalls; /* offset 122 */
    unsigned int cachedslot; /* offset 124 */
};
// clang-format on

/*
** Union of all collectible objects
*/
union GCObject
{
    GCheader gch;
    struct TString ts;
    struct Udata u;
    struct Closure cl;
    struct LuaTable h;
    struct Proto p;
    struct UpVal uv;
    struct lua_State th; // thread
    struct LuauBuffer buf;
    struct LuauClass lclass;
    struct LuauObject lobject;
};

// macros to convert a GCObject into a specific value
#define gco2ts(o) check_exp((o)->gch.tt == LUA_TSTRING, &((o)->ts))
#define gco2u(o) check_exp((o)->gch.tt == LUA_TUSERDATA, &((o)->u))
#define gco2cl(o) check_exp((o)->gch.tt == LUA_TFUNCTION, &((o)->cl))
#define gco2h(o) check_exp((o)->gch.tt == LUA_TTABLE, &((o)->h))
#define gco2p(o) check_exp((o)->gch.tt == LUA_TPROTO, &((o)->p))
#define gco2uv(o) check_exp((o)->gch.tt == LUA_TUPVAL, &((o)->uv))
#define gco2th(o) check_exp((o)->gch.tt == LUA_TTHREAD, &((o)->th))
#define gco2buf(o) check_exp((o)->gch.tt == LUA_TBUFFER, &((o)->buf))
#define gco2class(o) check_exp((o)->gch.tt == LUA_TCLASS, &((o)->lclass))
#define gco2object(o) check_exp((o)->gch.tt == LUA_TOBJECT, &((o)->lobject))

// macro to convert any Lua object into a GCObject
#define obj2gco(v) check_exp(iscollectable(v), cast_to(GCObject*, (v) + 0))

LUAI_FUNC lua_State* luaE_newthread(lua_State* L);
LUAI_FUNC void luaE_freethread(lua_State* L, lua_State* L1, struct lua_Page* page);
