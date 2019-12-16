// Fill out your copyright notice in the Description page of Project Settings.

#include "DebuggerSetting.h"
//#include "UnrealLua.h"
#include "RemoteDebuggerSetting.h"
#include "UnLuaDelegates.h"
#include "lua.hpp"
#include "LuaDebugger.h"
// #include "DebuggerVarNode.lua.h"


UDebuggerSetting* UDebuggerSetting::Get(bool IsRemoteDebugger)
{
	static UDebuggerSetting* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UDebuggerSetting>();
		Singleton->AddToRoot();

		//Bind UnLUa Create Lua_State delegate
		//FUnLuaDelegates::OnLuaStateCreated.AddUObject(Singleton, &UDebuggerSetting::RegisterLuaState);

		//FUnLuaDelegates::OnPostLuaContextCleanup.AddUObject(Singleton, &UDebuggerSetting::UnRegisterLuaState);
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
#if 0
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
#endif
}




void UDebuggerSetting::SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos)
{
	//FLuaDebuggerModule::Get()->SetStackData(Content, Lines, FilePaths, StackIndex, FuncInfos);
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
	UDebuggerSetting::Get()->GetVarsChildren(*this);

	NodeChildren.GenerateValueArray(OutChildren);
}


bool FDebuggerVarNode::IsEditable()
{
	return (VarType == LUA_TSTRING || VarType == LUA_TNUMBER || VarType == LUA_TBOOLEAN);
}


/*
LUA_GLUE_EXPAND_BEGIN(UDebuggerSetting)
LUA_GLUE_FUNCTION(EnterDebug)
LUA_GLUE_EXPAND_END(UDebuggerSetting)
*/

/********Hook Debug Statement********/

static const FString TempVarName("(*temporary)");

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

//#define UNLUA_DEBUG
#ifdef UNLUA_DEBUG
static unlua_Debug ur;
#else
static unlua_State ur;
#endif // UNLUA_DEBUG


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
	Continue,
	StepIn,
	StepOver,
	StepOut
};

static unlua_over u_over;

static unlua_out u_out;

static EHookMode hook_mode = EHookMode::Continue;

static void debugger_hook_c(lua_State *L, lua_Debug *ar);

/********Hook Debug Statement********/

void UDebuggerSetting::RegisterLuaState(lua_State* State)
{
	L = State;

#if 0

	UE_LOG(LogTemp, Log, TEXT("unlua_Debug RegisterLuaState"));

	lua_sethook(L, debugger_hook_c, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);

#else
	if (BreakPoints.Num() > 0)
	{
		//Bind Lua Hook
		lua_sethook(L, debugger_hook_c, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);

#ifdef UNLUA_DEBUG
		for (TMap<FString, TSet<int32>>::TIterator It(BreakPoints); It; ++It)
		{
			for (auto Line : It->Value)
			{
				UE_LOG(LogTemp, Log, TEXT("unlua_Debug BreakPoints file[%s] line[%i]"), *It->Key, Line);
			}
		}
#endif
	}
#endif

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
	// Clear Stack Info UseLess
	FLuaDebuggerModule::Get()->ClearStackInfo();

	if (bFullCleanup)
	{
		hook_mode = EHookMode::Continue;
		L = NULL;
	}
}

bool UDebuggerSetting::NameTranslate(int32 VarType, FString& VarName, int32 StackIndex)
{
	switch (VarType)
	{
	case LUA_TNUMBER:
		VarName = FString::Printf(TEXT("[%lf]"), lua_tonumber(L, StackIndex));
		return true;
	case LUA_TSTRING:
		VarName = UTF8_TO_TCHAR(lua_tostring(L, StackIndex));
		return true;
	}
	return false;
}


void UDebuggerSetting::ValueTranslate(int32 VarType, FString& VarValue, int32 StackIndex)
{
	switch (VarType)
	{
	case LUA_TNONE:
		VarValue = TEXT("LUA_TNONE");
		break;
	case LUA_TNIL:
		VarValue = TEXT("LUA_TNIL");
		break;
	case LUA_TBOOLEAN:
		VarValue = lua_toboolean(L, StackIndex) == 0 ? TEXT("false") : TEXT("true");
		VarValue = FString::Printf(TEXT("LUA_TBOOLEAN [%s]"), *VarValue);
		break;
	case LUA_TLIGHTUSERDATA:
		VarValue = TEXT("LUA_TLIGHTUSERDATA");
		break;
	case LUA_TNUMBER:
		VarValue = FString::Printf(TEXT("LUA_TNUMBER [%lf]"), lua_tonumber(L, StackIndex));
		break;
	case LUA_TSTRING:
		VarValue = FString::Printf(TEXT("LUA_TSTRING [%s]"), UTF8_TO_TCHAR(lua_tostring(L, StackIndex)));
		break;
	case LUA_TTABLE:
		VarValue = TEXT("LUA_TTABLE");
		break;
	case LUA_TFUNCTION:
		VarValue = FString::Printf(TEXT("LUA_TFUNCTION [%d]"), lua_tocfunction(L, StackIndex));
		break;
	case LUA_TUSERDATA:
		VarValue = FString::Printf(TEXT("LUA_TUSERDATA [%p]"), lua_touserdata(L, StackIndex));
		break;
	case LUA_TTHREAD:
		VarValue = TEXT("LUA_TTHREAD");
		break;
	case LUA_NUMTAGS:
		VarValue = TEXT("LUA_NUMTAGS");
		break;
	}
}

void UDebuggerSetting::IteraionTable(FDebuggerVarNode& InNode, int32 NameIndex)
{
	//UE_LOG(LogTemp, Log, TEXT("Itera Start [%d]"), lua_gettop(L));

	FString VarName;
	FString VarValue;
	lua_pushnil(L);

	//check is last index
	if (InNode.NameList.Num() == NameIndex)
	{
#if 1
		//UE_LOG(LogTemp, Log, TEXT("While Start [%d]"), lua_gettop(L));

		//Generate New Node
		while (lua_next(L, -2))
		{
			int32 NameType = lua_type(L, -2);

			//UE_LOG(LogTemp, Log, TEXT("Names Start [%d]"), lua_gettop(L));

			if (NameTranslate(NameType, VarName, -2))
			{
				//UE_LOG(LogTemp, Log, TEXT("Names Ended [%d]"), lua_gettop(L));

				if (!TempVarName.Equals(VarName))
				{

					//UE_LOG(LogTemp, Log, TEXT("Value Start [%d]"), lua_gettop(L));

					int ValueType = lua_type(L, -1);
					ValueTranslate(ValueType, VarValue, -1);


					//UE_LOG(LogTemp, Log, TEXT("Value Ended [%d]"), lua_gettop(L));

					if (!InNode.NodeChildren.Contains(VarName))
					{

						FDebuggerVarNode_Ref NewNode = MakeShareable(new FDebuggerVarNode);
						NewNode->NodeType = InNode.NodeType;
						NewNode->StackLevel = InNode.StackLevel;
						NewNode->VarType = ValueType;
						NewNode->VarName = FText::FromString(VarName);
						NewNode->VarValue = FText::FromString(VarValue);
						NewNode->NameList.Append(InNode.NameList);
						NewNode->NameList.Add(VarName);

						//UE_LOG(LogTemp, Log, TEXT("NewNode [%s]"), *NewNode->ToString());

						InNode.NodeChildren.Add(VarName, NewNode);
					}

				}
			}

			//UE_LOG(LogTemp, Log, TEXT("Poped Start [%d]"), lua_gettop(L));

			lua_pop(L, 1);

			//UE_LOG(LogTemp, Log, TEXT("Poped Ended [%d]"), lua_gettop(L));

			//break;
		}

		//UE_LOG(LogTemp, Log, TEXT("While Ended [%d]"), lua_gettop(L));
#endif
	}
	else
	{
#if 1
		// Continue IteraionTable
		while (lua_next(L, -2) != 0)
		{
			int32 NameType = lua_type(L, -2);

			if (NameTranslate(NameType, VarName, -2))
			{
				int ValueType = lua_type(L, -1);
				if (VarName.Equals(InNode.NameList[NameIndex]))
				{
					if (ValueType == LUA_TTABLE)
					{
						IteraionTable(InNode, NameIndex + 1);
					}
				}
			}

			lua_pop(L, 1);
		}
#endif
	}

	//lua_pop(L, 1);

	//UE_LOG(LogTemp, Log, TEXT("Itera Ended [%d]"), lua_gettop(L));
}

void UDebuggerSetting::LocalListen(FDebuggerVarNode& InNode)
{
	//UE_LOG(LogTemp, Log, TEXT("%s lua_top[%d]"), *InNode.ToString(), lua_gettop(L));

	//Get Local Vars
	lua_Debug ar;

	if (lua_getstack(L, InNode.StackLevel, &ar) != 0)
	{
		int i = 1;
		const char* VarName;
		FString VarValue;
		while ((VarName = lua_getlocal(L, &ar, i)) != NULL)
		{
			if (TempVarName.Equals(UTF8_TO_TCHAR(VarName)))
			{
				lua_pop(L, 1);
				i++;
				continue;
			}


			int32 VarType = lua_type(L, -1);
			ValueTranslate(VarType, VarValue, -1);

			if (InNode.NameList.Num() == 0)
			{
				if (!InNode.NodeChildren.Contains(VarName))
				{
					FDebuggerVarNode_Ref NewNode = MakeShareable(new FDebuggerVarNode);
					NewNode->NodeType = EVarNodeType::Local;
					NewNode->StackLevel = InNode.StackLevel;
					NewNode->VarType = VarType;
					NewNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarName));
					NewNode->VarValue = FText::FromString(VarValue);
					NewNode->NameList.Add(UTF8_TO_TCHAR(VarName));
					InNode.NodeChildren.Add(VarName, NewNode);
				}
			}
			else
			{
				if (InNode.NameList[0].Equals(UTF8_TO_TCHAR(VarName)))
				{
					if (VarType == LUA_TTABLE)
					{
						IteraionTable(InNode, 1);
						lua_pop(L, 1);
						break;
					}
				}
			}

			lua_pop(L, 1);

			i++;
		}
	}
}

void UDebuggerSetting::UpvalueListen(FDebuggerVarNode& InNode)
{
	int i = 1;
	const char* VarName;
	FString VarValue;
	while ((VarName = lua_getupvalue(L, -1, i)) != NULL)
	{

		if (TempVarName.Equals(UTF8_TO_TCHAR(VarName)))
		{
			lua_pop(L, 1);
			i++;
			continue;
		}

		int VarType = lua_type(L, -1);

		ValueTranslate(VarType, VarValue, -1);

		if (InNode.NameList.Num() == 0)
		{
			if (!InNode.NodeChildren.Contains(VarName))
			{
				FDebuggerVarNode_Ref NewNode = MakeShareable(new FDebuggerVarNode);
				NewNode->NodeType = EVarNodeType::UpValue;
				NewNode->StackLevel = InNode.StackLevel;
				NewNode->VarType = VarType;
				NewNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarName));
				NewNode->VarValue = FText::FromString(VarValue);
				NewNode->NameList.Add(UTF8_TO_TCHAR(VarName));
				InNode.NodeChildren.Add(VarName, NewNode);
			}
		}
		else
		{
			if (InNode.NameList[0].Equals(UTF8_TO_TCHAR(VarName)))
			{
				if (VarType == LUA_TTABLE)
				{
					IteraionTable(InNode, 1);
					lua_pop(L, 1);
					break;
				}
			}
		}

		lua_pop(L, 1);

		i++;
	}
}

void UDebuggerSetting::GlobalListen(FDebuggerVarNode& InNode)
{
	lua_pushglobaltable(L);
	lua_pushnil(L);
	const char* VarName;
	FString VarValue;
	int i = 0;
	while (lua_next(L, -2) != 0)
	{
		VarName = lua_tostring(L, -2);

		if (TempVarName.Equals(UTF8_TO_TCHAR(VarName)))
		{
			lua_pop(L, 1);
			i++;
			continue;
		}

		int VarType = lua_type(L, -1);

		ValueTranslate(VarType, VarValue, -1);

		if (InNode.NameList.Num() == 0)
		{
			if (!InNode.NodeChildren.Contains(VarName))
			{
				FDebuggerVarNode_Ref NewNode = MakeShareable(new FDebuggerVarNode);
				NewNode->NodeType = EVarNodeType::Global;
				NewNode->StackLevel = InNode.StackLevel;
				NewNode->VarType = VarType;
				NewNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarName));
				NewNode->VarValue = FText::FromString(VarValue);
				NewNode->NameList.Add(UTF8_TO_TCHAR(VarName));
				InNode.NodeChildren.Add(VarName, NewNode);
			}
		}
		else
		{
			if (InNode.NameList[0].Equals(UTF8_TO_TCHAR(VarName)))
			{
				if (VarType == LUA_TTABLE)
				{
					IteraionTable(InNode, 1);
					lua_pop(L, 1);
					break;
				}
			}
		}

		//UE_LOG(LogTemp, Log, TEXT("unlua_Debug Get Global index[%d] name[%s] value[%s]"), i, UTF8_TO_TCHAR(VarName), *VarValue);

		lua_pop(L, 1);
		i++;
	}
	lua_pop(L, 1);
}

void UDebuggerSetting::UEObjectListen()
{

}

void UDebuggerSetting::EnterDebug(const FString& LuaFilePath, int32 Line)
{
	//Collect Stack Info
	//UE_LOG(LogTemp, Log, TEXT("stackInfo Start"));
	TArray<TTuple<int32, int32, FString, FString>> StackInfos;
	int i = 0;
	lua_Debug ar;
	while (lua_getstack(L, i, &ar) != 0 && i < 10)
	{
		if (lua_getinfo(L, "Snl", &ar) != 0)
		{
			ur.Init(&ar);
			//UE_LOG(LogTemp, Log, TEXT("stackInfo [%d] %s"), i, *ur.ToString());
			TTuple<int32, int32, FString, FString> StackItem(i, ur.currentline, ur.source, ur.name);
			StackInfos.Add(StackItem);
		}
		i++;
	}
	//UE_LOG(LogTemp, Log, TEXT("stackInfo Ended"));
	FLuaDebuggerModule::Get()->SetStackData(StackInfos);

	FLuaDebuggerModule::Get()->EnterDebug(ur.source, ur.currentline);
}

void UDebuggerSetting::GetStackVars(int32 StackIndex, TArray<FDebuggerVarNode_Ref>& Vars)
{

	FDebuggerVarNode_Ref LocalNode = MakeShareable(new FDebuggerVarNode);
	LocalNode->NodeType = EVarNodeType::Local;
	LocalNode->StackLevel = StackIndex;
	LocalNode->VarName = FText::FromString("Local");

	FDebuggerVarNode_Ref UpValueNode = MakeShareable(new FDebuggerVarNode);
	UpValueNode->NodeType = EVarNodeType::UpValue;
	UpValueNode->StackLevel = StackIndex;
	UpValueNode->VarName = FText::FromString("UpValue");

	FDebuggerVarNode_Ref GlobalNode = MakeShareable(new FDebuggerVarNode);
	GlobalNode->NodeType = EVarNodeType::Global;
	GlobalNode->StackLevel = StackIndex;
	GlobalNode->VarName = FText::FromString("Global");

	FDebuggerVarNode_Ref UEObjectNode = MakeShareable(new FDebuggerVarNode);
	UEObjectNode->NodeType = EVarNodeType::UEObject;
	UEObjectNode->StackLevel = StackIndex;
	UEObjectNode->VarName = FText::FromString("UEObject");

	Vars.Add(LocalNode);
	Vars.Add(UpValueNode);
	Vars.Add(GlobalNode);
	Vars.Add(UEObjectNode);
}

void UDebuggerSetting::GetVarsChildren(FDebuggerVarNode& InNode)
{
	if (InNode.NameList.Num() > 0 && InNode.VarType != LUA_TTABLE)
		return;

	switch (InNode.NodeType)
	{
	case EVarNodeType::Local:
		LocalListen(InNode);
		break;
	case EVarNodeType::UpValue:
		UpvalueListen(InNode);
		break;
	case EVarNodeType::Global:
		GlobalListen(InNode);
		break;
	case EVarNodeType::UEObject:
		break;
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
	case Continue:
		if (FLuaDebuggerModule::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			UDebuggerSetting::Get()->EnterDebug(ur.source, ur.currentline);
		}
		break;
	case StepIn:
		UDebuggerSetting::Get()->EnterDebug(ur.source, ur.currentline);
		break;
	case StepOver:
		if (u_over.BreakPoint())
		{
			UDebuggerSetting::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		if (FLuaDebuggerModule::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			UDebuggerSetting::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	case StepOut:
		if (u_out.BreakPoint())
		{
			UDebuggerSetting::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		if (FLuaDebuggerModule::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			UDebuggerSetting::Get()->EnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	}
}

static void debugger_hook_c(lua_State *L, lua_Debug *ar)
{
	//UE_LOG(LogTemp, Log, TEXT("unlua_Debug debugger_hook_c"));

	if (lua_getinfo(L, "Snl", ar) != 0)
	{

#ifdef UNLUA_DEBUG
		ur.Init(ar);
		UE_LOG(LogTemp, Log, TEXT("%s"), *ur.ToString());
		if (ar->currentline < 0)
			return;
#else
		if (ar->currentline < 0)
			return;
		ur.Init(ar);
#endif // UNLUA_DEBUG

#undef UNLUA_DEBUG

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


