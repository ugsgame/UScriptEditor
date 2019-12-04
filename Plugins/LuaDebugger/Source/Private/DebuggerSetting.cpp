// Fill out your copyright notice in the Description page of Project Settings.

#include "DebuggerSetting.h"
//#include "UnrealLua.h"
#include "LuaDebugger.h"
#include "RemoteDebuggerSetting.h"
#include "UnLuaDelegates.h"
#include "lua.hpp"
// #include "DebuggerVarNode.lua.h"


UDebuggerSetting* UDebuggerSetting::Get(bool IsRemoteDebugger)
{
	static UDebuggerSetting* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UDebuggerSetting>();
		Singleton->AddToRoot();

		//Bind UnLUa Create Lua_State delegate
		FUnLuaDelegates::OnLuaStateCreated.AddUObject(Singleton, &UDebuggerSetting::RegisterLuaState);

		FUnLuaDelegates::OnPostLuaContextCleanup.AddUObject(Singleton, &UDebuggerSetting::UnRegisterLuaState);
	}
	return Singleton;
}

FString UDebuggerSetting::GetLuaSourceDir()
{
	return FLuaDebuggerModule::Get()->GetLuaSourceDir();
}

void UDebuggerSetting::PullDataToLua()
{
	UpdateBreakPoint(BreakPoints);
	ToggleDebugStart(bIsStart);
	SetTabIsOpen(bTabIsOpen);
}

void UDebuggerSetting::UpdateBreakPoint(TMap<FString, TSet<int32>>& BreakPoint)
{
	BreakPoints = BreakPoint;
	//	LuaCall_AllState("UpdateBreakPoint", this, BreakPoint);
}

void UDebuggerSetting::ToggleDebugStart(bool IsStart)
{
	bIsStart = IsStart;
	//	LuaCall_AllState("ToggleDebugStart", this, IsStart);
}

void UDebuggerSetting::SetTabIsOpen(bool IsOpen)
{
	bTabIsOpen = IsOpen;
	//	LuaCall_AllState("SetTabIsOpen", this, IsOpen);
}

void UDebuggerSetting::CombineNodeArr(TArray<FDebuggerVarNode_Ref>& PreVars, TArray<FDebuggerVarNode>& NowVars)
{
	TMap<FString, FDebuggerVarNode> NameToNowVarNode;
	TMap<FString, FDebuggerVarNode_Ref> NameToPreVarNode;

	for (auto& Node : NowVars)
	{
		NameToNowVarNode.Add(Node.Name.ToString(), Node);
	}

	for (auto& Node : PreVars)
	{
		FString NodeName(Node.Get().Name.ToString());
		NameToPreVarNode.Add(NodeName, Node);
		FDebuggerVarNode* NodePtr = NameToNowVarNode.Find(NodeName);
		if (NodePtr == nullptr)
		{
			Node.Get().Name = FText::FromString("nil");
			Node.Get().Value = FText::FromString("nil");
			Node.Get().ValueWeakIndex = -1;
			Node.Get().ValueChildren.Reset();
		}
		else
		{
			Node.Get().Name = NodePtr->Name;
			Node.Get().Value = NodePtr->Value;
			if (NodePtr->Value.ToString() == "nil")
			{
				Node.Get().ValueChildren.Reset();
				Node.Get().ValueWeakIndex = -1;
			}
			else
				Node.Get().ValueWeakIndex = NodePtr->ValueWeakIndex;
		}
	}

	for (auto& Node : NowVars)
	{
		FDebuggerVarNode_Ref* PreNodePtr = NameToPreVarNode.Find(Node.Name.ToString());
		if (PreNodePtr == nullptr)
		{
			Node.DebuggerSetting = this;
			PreVars.Add(MakeShareable(new FDebuggerVarNode(Node)));
		}
	}
}


void UDebuggerSetting::GetStackVars(int32 StackIndex, TArray<FDebuggerVarNode_Ref>& Vars)
{
	// 	TArray<FDebuggerVarNode> Result = LuaCallr_State(HookingLuaState, TArray<FDebuggerVarNode>, "GetStackVars", this, StackIndex);
	//	CombineNodeArr(Vars, Result);
}

void UDebuggerSetting::GetVarsChildren(FDebuggerVarNode InNode, TArray<FDebuggerVarNode_Ref>& OutChildren)
{
	/*
	TArray<FDebuggerVarNode> Result = LuaCallr_State(HookingLuaState, TArray<FDebuggerVarNode>, "GetVarNodeChildren", this, InNode);
	Result.StableSort([&](const FDebuggerVarNode& a, const FDebuggerVarNode& b)
	{
		bool aIsFunc = a.Value.ToString().Contains("function:");
		bool bIsFunc = b.Value.ToString().Contains("function:");
		if (aIsFunc != bIsFunc)
			return bIsFunc;
		FString a_KeyString = a.Name.ToString();
		FString b_KeyString = b.Name.ToString();
		if (a_KeyString.IsNumeric() != b_KeyString.IsNumeric())
		{
			return b_KeyString.IsNumeric();
		}
		else if (a_KeyString.IsNumeric())
		{
			return FCString::Atoi(*a_KeyString) < FCString::Atoi(*b_KeyString);
		}
		else
			return a.Name.CompareTo(b.Name)<0;
	});

	CombineNodeArr(OutChildren, Result);
	*/
}

void UDebuggerSetting::EnterDebug(lua_State* inL)
{
	/*
	HookingLuaState = inL;
	FString LuaFilePath = UTableUtil::pop<FString>(inL, 2);
	int32 Line = UTableUtil::pop<int32>(inL, 3);
	FLuaDebuggerModule::Get()->EnterDebug(LuaFilePath, Line);
	*/
}

void UDebuggerSetting::SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos)
{
	FLuaDebuggerModule::Get()->SetStackData(Content, Lines, FilePaths, StackIndex, FuncInfos);
}

FText UDebuggerSetting::GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine)
{
	return FLuaDebuggerModule::Get()->GetBreakPointHitConditionText(FilePath, CodeLine);
}


void UDebuggerSetting::BreakConditionChange()
{
	//	LuaCall_AllState("BreakConditionChange", this);
}


void FDebuggerVarNode::GetChildren(TArray<TSharedRef<FDebuggerVarNode>>& OutChildren)
{
	if (DebuggerSetting && ValueWeakIndex > 0)
		DebuggerSetting->GetVarsChildren(*this, ValueChildren);
	OutChildren = ValueChildren;
}


/*
LUA_GLUE_EXPAND_BEGIN(UDebuggerSetting)
LUA_GLUE_FUNCTION(EnterDebug)
LUA_GLUE_EXPAND_END(UDebuggerSetting)
*/

/********Hook Debug Statement********/

struct unlua_State
{
	FString source;
	int32 currentline;

	void Init(lua_Debug* ar)
	{
		FString sourcePath = UTF8_TO_TCHAR(++ar->source);
		source = sourcePath.Replace(*FString("\\"), *FString("/"));
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

//static unlua_Debug ur;
static unlua_State ur;

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

enum EHookMode
{
	Start,
	Continue,
	StepIn,
	StepOver,
	StepOut
};

static unlua_over u_over;

static unlua_out u_out;

static EHookMode hook_mode = EHookMode::Start;

static void debugger_hook_c(lua_State *L, lua_Debug *ar);

/********Hook Debug Statement********/

void UDebuggerSetting::RegisterLuaState(lua_State* State)
{
	L = State;
	if (BreakPoints.Num() > 0)
	{
		lua_sethook(L, debugger_hook_c, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
		//set hook mode
		hook_mode = EHookMode::Start;

#if 0
		for (TMap<FString, TSet<int32>>::TIterator It(BreakPoints); It; ++It)
		{
			for (auto Line : It->Value)
			{
				UE_LOG(LogTemp, Log, TEXT("unlua_Debug BreakPoints file[%s] line[%i]"), *It->Key, Line);
			}
		}
#endif

	}
	else
		hook_mode = EHookMode::Continue;
}

void UDebuggerSetting::UnRegisterLuaState(bool bFullCleanup)
{
	if (bFullCleanup)
	{
		UE_LOG(LogTemp, Log, TEXT("unlua_Debug UnRegisterLuaState FullCleanup"));
	}
	else
		UE_LOG(LogTemp, Log, TEXT("unlua_Debug UnRegisterLuaState Not FullCleanup"));

	lua_sethook(L, NULL, 0, 0);
	if (bFullCleanup)
	{
		hook_mode = EHookMode::Start;
		L = NULL;
	}
}

void UDebuggerSetting::Continue()
{
	if (L == NULL)
		return;
	lua_sethook(L, NULL, 0, 0);
	hook_mode = EHookMode::Continue;
}

void UDebuggerSetting::StepOver()
{
	if (L == NULL)
		return;

	u_over.Reset();
	hook_mode = EHookMode::StepOver;

}

void UDebuggerSetting::StepIn()
{
	if (L == NULL)
		return;
	hook_mode = EHookMode::StepIn;
}

void UDebuggerSetting::StepOut()
{
	if (L == NULL)
		return;

	u_out.Reset();
	hook_mode = EHookMode::StepOut;

}



static void hook_call_option(lua_State* L)
{
	switch (hook_mode)
	{
	case Continue:
		break;
	case StepIn:
		break;
	case StepOver:
		u_over.CallOper();
		break;
	case StepOut:
		u_out.CallOper();
		break;
	}
}

static void hook_ret_option(lua_State* L)
{
	switch (hook_mode)
	{
	case Continue:
		break;
	case StepIn:
		break;
	case StepOver:
		u_over.RetOper();
		break;
	case StepOut:
		u_out.RetOper();
		break;
	}
}

static void hook_line_option(lua_State* L)
{
	switch (hook_mode)
	{
	case Start:
		if (FLuaDebuggerModule::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
		}
		break;
	case Continue:
		break;
	case StepIn:
		FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
		break;
	case StepOver:
		if (u_over.BreakPoint())
		{
			FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		if (FLuaDebuggerModule::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	case StepOut:
		if (u_out.BreakPoint())
		{
			FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		if (FLuaDebuggerModule::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	}
}

static void debugger_hook_c(lua_State *L, lua_Debug *ar)
{
	if (lua_getinfo(L, "Snl", ar) != 0)
	{

		//UE_LOG(LogTemp, Log, TEXT("%s"), *ur.ToString());

		//if currentline is -1, do nothing
		if (ar->currentline < 0)
			return;

		ur.Init(ar);

		switch (ar->event)
		{
		case LUA_HOOKCALL:
			hook_call_option(L);
			break;
		case LUA_HOOKRET:
			hook_ret_option(L);
			break;
		case LUA_HOOKLINE:
			hook_line_option(L);
			break;
		}
	}
}