#pragma once

#include "LuaCore.h"
/**
* Extern Lua global functions for UScriptEditor
*/
UNLUA_API int32 Global_CheckModule(lua_State *L);
UNLUA_API int32 Global_RequireModule(lua_State *L);
UNLUA_API int32 Global_LoadContext(lua_State *L);
