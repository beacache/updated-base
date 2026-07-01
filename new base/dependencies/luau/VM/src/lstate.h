// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lobject.h"
#include "ltm.h"
#include "ludata.h"

// registry
#define registry(L) (&L->global->registry)

// extra stack space to handle TM calls and some other extras
#define EXTRA_STACK 5

#define BASIC_CI_SIZE 8

#define BASIC_STACK_SIZE (2 * LUA_MINSTACK)

// clang-format off
struct stringtable 
{
        TString** hash; // 0x0
        int size;       // 0x8  
        int nuse;       // 0xC  
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
typedef struct CallInfo
{
    TValue* top;        // 0x00  frame top
    union
    {
        const Instruction* savedpc; // 0x08  
        int                errfunc; // 0x08  
    };
    TValue* base;       // 0x10  
    TValue* func;       // 0x18  
    int                nresults;   // 0x20
    unsigned int       flags;      // 0x24 
    Proto* p;                   // 0x28 
} CallInfo; // sizeof == 0x30
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

#ifdef LUAI_GCMETRICS
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
#endif

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
typedef struct global_State
{
    unsigned char currentwhite;     // 0x00
    unsigned char gcstate;          // 0x01
    unsigned char gap_pre_frealloc[0x6]; // 0x02
    lua_Alloc frealloc;             // 0x08
    void* ud;                       // 0x10
    GCObject* weak;                 // 0x18
    GCObject* grayagain;            // 0x20
    GCObject* gray;                 // 0x28
    size_t GCthreshold;             // 0x30
    size_t totalbytes;              // 0x38
    int gcstepsize;                 // 0x40
    int gcstepmul;                  // 0x44
    int gcgoal;                     // 0x48
    struct stringtable strt;        // 0x50
    lua_Page* sweepgcopage;         // 0x60 
    lua_Page* freegcopages[40];     // 0x68 
    lua_Page* freepages[40];        // 0x1A8
    lua_State* mainthread;          // 0x2E8
    UpVal uvhead;                   // 0x2F0
    lua_Page* allpages;             // 0x318
    lua_Page* allgcopages;          // 0x320
    LuaTable* mt[14];               // 0x328
    TString* ttname[14];            // 0x398
    TString* tmname[21];            // 0x408
    TValue pseudotemp;              // 0x4B0
    TValue registry;                // 0x4C0
    struct registryfree_value
    {
    private:
        int _value;

    public:
        void operator=(const registryfree_value& value)
        {
            this->operator=(value);
        }

        void operator=(const int& value)
        {
            _value = (_value & 0xF0000000) | (value & 0xFFFFFFF);
        }

        operator const int() const
        {
            return _value & 0xFFFFFFF;
        }

        bool operator==(const int& value) const
        {
            return (_value & 0xFFFFFFF) == value;
        }

        bool operator!=(const int& value) const
        {
            return (_value & 0xFFFFFFF) != value;
        }
    };
    struct registryfree_value; // the entire struct definition
    // int by default
    registryfree_value registryfree; // 0x4D0 Next free slot in registry
    unsigned char gap_pre_cb[0x4];  // 0x4D4
    lua_Callbacks cb;               // 0x4D8
    uint64_t rngstate;              // 0x528
    struct lua_jmpbuf* errorjmp;    // 0x530
    uint64_t ptrenckey[4];          // 0x538 
    lua_ExecutionCallbacks ecb;     // 0x558 
    alignas(16) unsigned char ecbdata[512]; // 0x5A0
    lua_UdataDirectAccessData udatadirect[130]; // 0x7A0
    size_t memcatbytes[256];        // 0x2C30
    void (*udatagc[128])(struct lua_State*, void*); // 0x3430
    LuaTable* udatamt[128];         // 0x3830

    TString* lightuserdataname[128]; // 0x3C30
    struct LuaTable* udatadirectfields[130]; // 0x4030
    GCStats gcstats;      // 0x4440
    uint32_t lastprotoid; // 0x44F8

#ifdef LUAI_GCMETRICS
    GCMetrics gcmetrics; // 0x4500
#endif
} global_State;
// clang-format on

/*
** `per thread' state
*/
// clang-format off
struct lua_State
{
    CommonHeader;
    uint8_t status; // 0x3
    bool singlestep; // 0x4
    bool isactive; // 0x5
    uint8_t activememcat; // 0x6
    GCObject* gclist; // 0x8
    RobloxExtraSpace* userdata; // 0x10
    LuaTable* gt; // 0x18
    unsigned short nCcalls; // 0x20
    unsigned short baseCcalls; // 0x22
    int cachedslot; // 0x24
    CallInfo* end_ci; // 0x28
    CallInfo* base_ci; // 0x30
    StkId top; // 0x38 
    StkId base; // 0x40
    CallInfo* ci; // 0x48
    StkId stack_last; // 0x50
    StkId stack; // 0x58
    global_State* global; // 0x60
    UpVal* openupval; // 0x68
    TString* namecall; // 0x70
    lstate_stacksize<int> stacksize; // 0x78
    int size_ci; // 0x7C
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
