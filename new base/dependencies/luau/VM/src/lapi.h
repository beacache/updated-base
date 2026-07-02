// lapi.h - revert to original, no changes needed
#pragma once

#include "lobject.h"

LUAI_FUNC const TValue* luaA_toobject(lua_State* L, int idx);
LUAI_FUNC TValue* index2addr(lua_State* L, int idx); // remove static
LUAI_FUNC void luaA_pushvalue(lua_State* L, const TValue* o);
LUAI_FUNC void luaA_pushclass(lua_State* L, LuauClass* lclass);