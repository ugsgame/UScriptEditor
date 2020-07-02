
#include "UnLuaExtend.h"
#include "UnLua.h"
#include "UnLuaDelegates.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnLuaInterface.h"
#include "DefaultParamCollection.h"

extern "C"
{
#include "lfunc.h"
#include "lstate.h"
#include "lobject.h"
}

UNLUA_API int32 Global_LoadContext(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	const char *ModuleName = lua_tostring(L, 1);
	if (!ModuleName)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	bool LoadString = true;	//Otherwise load buffer, false
	if (NumParams >= 2)
	{
		LoadString = (bool)lua_toboolean(L, 2);
	}

	bool LoadWithContext = false;
	FModuleContext CodeContext;
	if (GModuleContext.Path.Len() < 1 && GModuleContext.SourceCode.Len() < 1 && GModuleContext.ByteCode.Num() < 1)
	{
		if (FUnLuaDelegates::LoadModuleContext.IsBound())
		{
			LoadWithContext = FUnLuaDelegates::LoadModuleContext.Execute(ModuleName, CodeContext);
			if (LoadWithContext)
			{
				GModuleContext = CodeContext;
			}
		}
	}


	if (LoadString && GModuleContext.SourceCode.Len() < 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module context:source code len < 1"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}
	if (!LoadString && GModuleContext.ByteCode.Num() < 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module context:byte code len < 1"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}


	lua_settop(L, 1);       /* LOADED table will be at index 2 */

	lua_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
	lua_getfield(L, 2, ModuleName);
	if (lua_toboolean(L, -1))
	{
		return 1;
	}

	lua_pop(L, 1);

	bool bSuccess = false;

	FString RelativeFilePath = GModuleContext.Path;
	if (LoadString)
	{
		TArray<TCHAR>  CharArray = GModuleContext.SourceCode.GetCharArray();
		for (int32 i = 0; i < CharArray.Num(); i++)
		{
			if (CharArray[i] == '\0')
			{
				CharArray.RemoveAt(i);
				i--;
			}
		}
		//bSuccess = UnLua::LoadString(L, TCHAR_TO_UTF8(*GCodeContext.SourceCode));
		bSuccess = UnLua::LoadString(L, TCHAR_TO_UTF8(CharArray.GetData()));
	}
	else
	{
		TArray<uint8> Data = GModuleContext.ByteCode;
		//////////////////////////////////////////////////////////////////////////
		//remove '\0'
		/*
		for (int32 i = 0; i < Data.Num(); i++)
		{
			if (Data[i] == '\0')
			{
				Data.RemoveAt(i);
				i--;
			}
		}
		*/
		//////////////////////////////////////////////////////////////////////////
		int32 SkipLen = (3 < Data.Num()) && (0xEF == Data[0]) && (0xBB == Data[1]) && (0xBF == Data[2]) ? 3 : 0;        // skip UTF-8 BOM mark
		bSuccess = UnLua::LoadChunk(L, (const char*)(Data.GetData() + SkipLen), Data.Num() - SkipLen, TCHAR_TO_ANSI(*RelativeFilePath), "bt", 0);    // loads the buffer as a Lua chunk
	}

	//Reset golbal CodeContext;
	GModuleContext.Empty();

	if (!bSuccess)
	{
		
		UE_LOG(LogUnLua, Log, TEXT("%s: load module fail: %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(ModuleName));

		const char * name = ModuleName;
		//FindLuaLoader(L, name);
		lua_pushstring(L, name);  // pass name as argument to module loader 
		lua_insert(L, -2);  // name is 1st argument (before search data) 
		lua_call(L, 2, 1);  // run loader to load module 
		if (!lua_isnil(L, -1))  // non-nil return? 
			lua_setfield(L, 2, name);  // LOADED[name] = returned value 
		if (lua_getfield(L, 2, name) == LUA_TNIL) {   // module set no value? 
			lua_pushboolean(L, 1);  // use true as result 
			lua_pushvalue(L, -1);  // extra copy to be returned 
			lua_setfield(L, 2, name);  // LOADED[name] = true 
		}
		
		return 1;
	}

	FString FullFilePath = GLuaSrcFullPath + RelativeFilePath;
	lua_pushvalue(L, 1);
	lua_pushstring(L, TCHAR_TO_UTF8(*FullFilePath));
	lua_pcall(L, 2, 1, 0);

	if (!lua_isnil(L, -1))
	{
		lua_setfield(L, 2, ModuleName);
	}
	if (lua_getfield(L, 2, ModuleName) == LUA_TNIL)
	{
		lua_pushboolean(L, 1);
		lua_pushvalue(L, -1);
		lua_setfield(L, 2, ModuleName);
	}
	return 1;
}

UNLUA_API int32 Global_RequireModule(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid parameters!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	const char *ModuleName = lua_tostring(L, 1);
	if (!ModuleName)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module name!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	bool LoadString = true;	//Otherwise load buffer, false
	if (NumParams >= 2)
	{
		LoadString = (bool)lua_toboolean(L, 2);
	}

	FModuleContext CodeContext;
	if (FUnLuaDelegates::LoadModuleContext.IsBound())
	{
		FUnLuaDelegates::LoadModuleContext.Execute(ModuleName, CodeContext);
	}

	if (LoadString && CodeContext.SourceCode.Len() < 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module context:source code len < 1"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}
	if (!LoadString && CodeContext.ByteCode.Num() < 1)
	{
		UNLUA_LOGERROR(L, LogUnLua, Log, TEXT("%s: Invalid module context:byte code len < 1"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}


	lua_settop(L, 1);       /* LOADED table will be at index 2 */

	lua_getfield(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
	lua_getfield(L, 2, ModuleName);
	if (lua_toboolean(L, -1))
	{
		return 1;
	}

	lua_pop(L, 1);

	bool bSuccess = false;

	FString RelativeFilePath = CodeContext.Path;
	if (LoadString)
	{
		TArray<TCHAR>  CharArray = CodeContext.SourceCode.GetCharArray();
		for (int32 i = 0; i < CharArray.Num(); i++)
		{
			if (CharArray[i] == '\0')
			{
				CharArray.RemoveAt(i);
				i--;
			}
		}
		//bSuccess = UnLua::LoadString(L, TCHAR_TO_UTF8(*GCodeContext.SourceCode));
		bSuccess = UnLua::LoadString(L, TCHAR_TO_UTF8(CharArray.GetData()));
	}
	else
	{
		TArray<uint8> Data = CodeContext.ByteCode;
		//////////////////////////////////////////////////////////////////////////
		//remove '\0'
		/*
		for (int32 i = 0; i < Data.Num(); i++)
		{
			if (Data[i] == '\0')
			{
				Data.RemoveAt(i);
				i--;
			}
		}
		*/
		//////////////////////////////////////////////////////////////////////////
		int32 SkipLen = (3 < Data.Num()) && (0xEF == Data[0]) && (0xBB == Data[1]) && (0xBF == Data[2]) ? 3 : 0;        // skip UTF-8 BOM mark
		bSuccess = UnLua::LoadChunk(L, (const char*)(Data.GetData() + SkipLen), Data.Num() - SkipLen, TCHAR_TO_ANSI(*RelativeFilePath), "bt", 0);    // loads the buffer as a Lua chunk
	}


	if (!bSuccess)
	{
		
		UE_LOG(LogUnLua, Log, TEXT("%s: Require module fail: %s!"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(ModuleName));

		const char * name = ModuleName;
		//FindLuaLoader(L, name);
		lua_pushstring(L, name);  // pass name as argument to module loader
		lua_insert(L, -2);  // name is 1st argument (before search data)
		lua_call(L, 2, 1);  // run loader to load module 
		if (!lua_isnil(L, -1))  // non-nil return? 
			lua_setfield(L, 2, name);  // LOADED[name] = returned value
		if (lua_getfield(L, 2, name) == LUA_TNIL) {   // module set no value? 
			lua_pushboolean(L, 1);  // use true as result 
			lua_pushvalue(L, -1);  // extra copy to be returned 
			lua_setfield(L, 2, name);  // LOADED[name] = true 
		}
		return 1;
	}

	FString FullFilePath = GLuaSrcFullPath + RelativeFilePath;
	lua_pushvalue(L, 1);
	lua_pushstring(L, TCHAR_TO_UTF8(*FullFilePath));
	lua_pcall(L, 2, 1, 0);

	if (!lua_isnil(L, -1))
	{
		lua_setfield(L, 2, ModuleName);
	}
	if (lua_getfield(L, 2, ModuleName) == LUA_TNIL)
	{
		lua_pushboolean(L, 1);
		lua_pushvalue(L, -1);
		lua_setfield(L, 2, ModuleName);
	}
	return 1;
}

UNLUA_API int32 Global_CheckModule(lua_State *L)
{

	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	const char *ModuleName = lua_tostring(L, 1);
	if (!ModuleName)
	{
		lua_pushboolean(L, 0);
		return 1;
	}

	FString FileName(ANSI_TO_TCHAR(ModuleName));
	FileName.ReplaceInline(TEXT("."), TEXT("/"));
	FString RelativeFilePath = FString::Printf(TEXT("%s.lua"), *FileName);
	FString FullFilePath = GLuaSrcFullPath + RelativeFilePath;

	IFileManager::Get().FileExists(*FullFilePath) ? lua_pushboolean(L, 1) : lua_pushboolean(L, 0);
	return 1;
}