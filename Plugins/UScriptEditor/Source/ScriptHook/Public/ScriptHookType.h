// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "lua.hpp"
#include "STreeView.h"
#include "ScriptHookType.generated.h"

struct lua_Debug;
struct lua_State;


UENUM()
enum class EScriptVarNodeType : uint8
{
	V_Local = 0,
	V_UpValue,
	V_Global,
	V_UEObject
};

UENUM()
enum class EScriptUEKindType : uint8
{
	T_Single = 0, // Single do not have child
	T_TList,      // Not key->value
	T_TDict,      // Has key->value
	T_UObject,
	T_AActor,
	T_UEObject,
	T_ClassDesc,
	T_UFunction,
};


enum EHookMode
{
	H_Continue,
	H_StepIn,
	H_StepOver,
	H_StepOut
};


enum EDealOrder
{
	O_Default = 0,
	O_InitData,
	O_ClientExit,
	O_BreakPoints,
	O_EnterDebug,
	O_ReqStack,
	O_StackVars,
	O_ReqChild,
	O_ChildVars,
	O_Continue,
	O_StepIn,
	O_StepOver,
	O_StepOut,
};


#if 0

T_bool,
T_uint8,
T_int32,
T_int64,
T_float,
T_FName,
T_FString,
T_FText,
T_Vector,
T_Rotator,
T_UFunction_Arg,

#endif


struct unlua_Remote
{
	FString source;
	FString name;
	int32 currentline;

	void Init(lua_Debug* ar)
	{
		FString sourcePath = UTF8_TO_TCHAR(ar->source);
		source = sourcePath.Replace(*FString("\\"), *FString("/"));
		name = UTF8_TO_TCHAR(ar->name);
		currentline = ar->currentline;
	}

	FString ToString(bool IsShort = true)
	{
		return FString::Printf(TEXT("unlua_Debug currentline[%d] ; source[%s]"), currentline, *source);
	}
};

struct unlua_State
{
	FString source;
	FString name;
	int32 currentline;

	void Init(lua_Debug* ar)
	{
		FString sourcePath = UTF8_TO_TCHAR(++ar->source);
		source = sourcePath.Replace(*FString("\\"), *FString("/"));
		name = UTF8_TO_TCHAR(ar->name);
		currentline = ar->currentline;
	}

	FString ToString(bool IsShort = true)
	{
		return FString::Printf(TEXT("unlua_Debug currentline[%d] ; source[%s]"), currentline, *source);
	}
};

struct unlua_Debug
{
	int32 event;
	FString source;
	FString short_src;
	int32 linedefined;
	int32 lastlinedefined;
	FString what;
	FString name;
	FString namewhat;
	int32 currentline;

	void Init(lua_Debug* ar)
	{
		event = ar->event;
		FString sourcePath = UTF8_TO_TCHAR(++ar->source);
		source = sourcePath.Replace(*FString("\\"), *FString("/"));
		short_src = UTF8_TO_TCHAR(ar->short_src);
		linedefined = ar->linedefined;
		lastlinedefined = ar->lastlinedefined;
		what = UTF8_TO_TCHAR(ar->what);
		name = UTF8_TO_TCHAR(ar->name);
		namewhat = UTF8_TO_TCHAR(ar->namewhat);
		currentline = ar->currentline;
	}

	FString ToString(bool IsShort = true)
	{
		if (IsShort)
			return FString::Printf(TEXT("unlua_Debug event[%s] ; currentline[%d] ; source[%s] ; name[%s] ; namewhat[%s]"), *EventFormat(), currentline, *source, *name, *namewhat);
		return FString::Printf(TEXT("unlua_Debug event[%s] ; currentline[%d] ; source[%s] ; name[%s] ; namewhat[%s] ; linedefined[%d] ; lastlinedefined[%d] ; short_src[%s] ; what[%s]"), *EventFormat(), currentline, *source, *name, *namewhat, linedefined, lastlinedefined, *short_src, *what);
	}

	FString EventFormat()
	{
		switch (event)
		{
		case LUA_HOOKCALL:
			return FString("Call");
		case LUA_HOOKRET:
			return FString("Retu");
		case LUA_HOOKLINE:
			return FString("Line");
		case LUA_HOOKCOUNT:
			return FString("Cout");
		case LUA_HOOKTAILCALL:
			return FString("Tail");
		}
		return FString("None");
	}
};


struct unlua_over
{
	int32 level;

	void Reset()
	{
		level = 0;
	}

	void CallOper()
	{
		level++;
	}

	void RetOper()
	{
		level--;
	}

	bool BreakPoint()
	{
		return level == 0 || level == -1;
	}
};

struct unlua_out {

	int32 level;

	void Reset()
	{
		level = 1;
	}

	void CallOper()
	{
		level++;
	}

	void RetOper()
	{
		level--;
	}

	bool BreakPoint()
	{
		return level == 0;
	}

};


USTRUCT()
struct FScriptDebuggerVarNode
{
	GENERATED_BODY()

public:

	FScriptDebuggerVarNode() {}

	UPROPERTY()
		TArray<int32> NodeMask;

	UPROPERTY()
		EScriptVarNodeType NodeType;

	UPROPERTY()
		int32 StackLevel;

	UPROPERTY()
		int32 KindType;

	UPROPERTY()
		FText VarName;

	UPROPERTY()
		FText VarValue;

	UPROPERTY()
		FText VarType;

	// cast by KindType to get value 
	void* VarPtr;

	UPROPERTY()
		TArray<FString> NameList;

	TMap<FString, TSharedRef<FScriptDebuggerVarNode>> NodeChildren;

	void GetChildren(TArray<TSharedRef<FScriptDebuggerVarNode>>& OutChildren);

	FString ToString()
	{
		//FString NameListString;
		//for (auto NameStr : NameList)
		//{
		//	NameListString += FString("-->").Append(NameStr);
		//}
		FString MaskListString;
		for (auto MaskIndex : NodeMask)
		{
			MaskListString += FString::Printf(TEXT("-> %d "), MaskIndex);
		}
		return FString::Printf(TEXT(" VarNode VarName[%s] MaskList[%s]"), *VarName.ToString(), *MaskListString);
	}

	FString ToMask()
	{
		FString MaskStr = FString::FromInt(NodeMask[0]);
		for (int i = 1; i < NodeMask.Num(); ++i)
		{
			MaskStr += FString::Printf(TEXT(",%d"), NodeMask[i]);
		}
		return MaskStr;
	}

};

using FScriptDebuggerVarNode_Ref = TSharedRef<FScriptDebuggerVarNode>;
using SDebuggerVarTree = STreeView<FScriptDebuggerVarNode_Ref>;

UENUM()
enum class EBreakPointState : uint8
{
	On = 0,
	Off = 1,
	Hit = 2,
	Disable = 3,
};


USTRUCT()
struct FScriptBreakPointNode
{
	GENERATED_BODY()
		FScriptBreakPointNode() {};
	FScriptBreakPointNode(int32 _Line, FString _FilePath)
		:FilePath(_FilePath), Line(_Line)
	{
		State = EBreakPointState::On;
	}
	UPROPERTY()
		FString FilePath;

	UPROPERTY()
		int32 Line;

	UPROPERTY()
		FText HitCondition;

	UPROPERTY()
		EBreakPointState State;
};