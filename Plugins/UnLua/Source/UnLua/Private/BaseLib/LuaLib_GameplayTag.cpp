// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "LuaCore.h"
#include "UnLuaEx.h"
#include "GameplayTagsManager.h"

static int32 FGameplayTag_New(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	void *Userdata = NewTypedUserdata(L, FGameplayTag);
	FGameplayTag* V = new(Userdata) FGameplayTag();
	const char *TagName = lua_tostring(L, 2);
	*V = UGameplayTagsManager::Get().RequestGameplayTag(FName(TagName));
	return 1;
}

static int32 FGameplayTag_Equals(lua_State* L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* A = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!A)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* B = (FGameplayTag*)GetCppInstanceFast(L, 2);
	if (!B)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	bool bEquals = (*A == *B);
	lua_pushboolean(L, bEquals);
	return 1;
}

static int32 FGameplayTag_LessThan(lua_State* L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* A = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!A)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* B = (FGameplayTag*)GetCppInstanceFast(L, 2);
	if (!B)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}
	bool bLessThan = (*A < *B);
	lua_pushboolean(L, bLessThan);
	return 1;
}

static int32 FGameplayTag_ToString(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams != 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* V = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!V)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	lua_pushstring(L, TCHAR_TO_ANSI(*V->ToString()));
	return 1;
}

static int32 FGameplayTag_IsValid(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams != 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* V = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!V)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	lua_pushboolean(L, V->IsValid());
	return 1;
}

static int32 FGameplayTag_RequestDirectParent(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* A = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!A)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	void *Userdata = NewTypedUserdata(L, FGameplayTag);
	FGameplayTag* B = new(Userdata) FGameplayTag();
	*B = A->RequestDirectParent();
	
	return 1;
}

static int32 FGameplayTag_MatchesTag(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* A = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!A)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* B = (FGameplayTag*)GetCppInstanceFast(L, 2);
	if (!B)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	bool bMatchesTag = A->MatchesTag(*B);
	lua_pushboolean(L, bMatchesTag);
	return 1;
}

static int32 FGameplayTag_MatchesTagDepth(lua_State *L)
{
	int32 NumParams = lua_gettop(L);
	if (NumParams < 1)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid parameters for __tostring!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* A = (FGameplayTag*)GetCppInstanceFast(L, 1);
	if (!A)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	FGameplayTag* B = (FGameplayTag*)GetCppInstanceFast(L, 2);
	if (!B)
	{
		UE_LOG(LogUnLua, Log, TEXT("%s: Invalid GameplayTag!"), ANSI_TO_TCHAR(__FUNCTION__));
		return 0;
	}

	lua_pushinteger(L, A->MatchesTagDepth(*B));
	return 1;
}


static const luaL_Reg FGameplayTagLib[] =
{
	{ "__call", FGameplayTag_New },
	{ "__eq", FGameplayTag_Equals },
	{ "__lt", FGameplayTag_LessThan },
	{ "__tostring", FGameplayTag_ToString },
	{ "IsValid", FGameplayTag_IsValid },
	{ "RequestDirectParent", FGameplayTag_RequestDirectParent },
	{ "MatchesTag", FGameplayTag_MatchesTag },
	{ "MatchesTagDepth", FGameplayTag_MatchesTagDepth },
	{ nullptr, nullptr }
};

BEGIN_EXPORT_REFLECTED_CLASS(FGameplayTag)
ADD_LIB(FGameplayTagLib)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FGameplayTag)
