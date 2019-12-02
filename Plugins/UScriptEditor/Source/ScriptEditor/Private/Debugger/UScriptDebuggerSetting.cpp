// Fill out your copyright notice in the Description page of Project Settings.

#include "UScriptDebuggerSetting.h"
//#include "UnrealLua.h"
#include "SScriptDebugger.h"
#include "UScriptRemoteDebuggerSetting.h"
// #include "DebuggerVarNode.lua.h"

UScriptDebuggerSetting* UScriptDebuggerSetting::Get(bool IsRemoteDebugger)
{
	if (IsRemoteDebugger == false)
	{
		static UScriptDebuggerSetting* Singleton = nullptr;
		if (Singleton == nullptr)
		{
			Singleton = NewObject<UScriptDebuggerSetting>();
			Singleton->AddToRoot();
		}
		return Singleton;
	}
	else
	{
		static UScriptRemoteDebuggerSetting* Singleton = nullptr;
		if (Singleton == nullptr)
		{
			Singleton = NewObject<UScriptRemoteDebuggerSetting>();
			Singleton->AddToRoot();
		}
		return Singleton;
	}
}

FString UScriptDebuggerSetting::GetLuaSourceDir()
{
	return SScriptDebugger::Get()->GetLuaSourceDir();
}

void UScriptDebuggerSetting::PullDataToLua()
{
	UpdateBreakPoint(BreakPoints);
	ToggleDebugStart(bIsStart);
	SetTabIsOpen(bTabIsOpen);
}

void UScriptDebuggerSetting::UpdateBreakPoint(TMap<FString, TSet<int32>>& BreakPoint)
{
	BreakPoints = BreakPoint;
	//	LuaCall_AllState("UpdateBreakPoint", this, BreakPoint);
}

void UScriptDebuggerSetting::ToggleDebugStart(bool IsStart)
{
	bIsStart = IsStart;
	//	LuaCall_AllState("ToggleDebugStart", this, IsStart);
}

void UScriptDebuggerSetting::SetTabIsOpen(bool IsOpen)
{
	bTabIsOpen = IsOpen;
	//	LuaCall_AllState("SetTabIsOpen", this, IsOpen);
}

void UScriptDebuggerSetting::CombineNodeArr(TArray<FDebuggerVarNode_Ref>& PreVars, TArray<FScriptDebuggerVarNode>& NowVars)
{
	TMap<FString, FScriptDebuggerVarNode> NameToNowVarNode;
	TMap<FString, FDebuggerVarNode_Ref> NameToPreVarNode;

	for (auto& Node : NowVars)
	{
		NameToNowVarNode.Add(Node.Name.ToString(), Node);
	}

	for (auto& Node : PreVars)
	{
		FString NodeName(Node.Get().Name.ToString());
		NameToPreVarNode.Add(NodeName, Node);
		FScriptDebuggerVarNode* NodePtr = NameToNowVarNode.Find(NodeName);
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
			PreVars.Add(MakeShareable(new FScriptDebuggerVarNode(Node)));
		}
	}
}

void UScriptDebuggerSetting::GetStackVars(int32 StackIndex, TArray<FDebuggerVarNode_Ref>& Vars)
{
	// 	TArray<FDebuggerVarNode> Result = LuaCallr_State(HookingLuaState, TArray<FDebuggerVarNode>, "GetStackVars", this, StackIndex);
	//	CombineNodeArr(Vars, Result);
}

void UScriptDebuggerSetting::GetVarsChildren(FScriptDebuggerVarNode InNode, TArray<FDebuggerVarNode_Ref>& OutChildren)
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

void UScriptDebuggerSetting::EnterDebug(lua_State* inL)
{
	/*
	HookingLuaState = inL;
	FString LuaFilePath = UTableUtil::pop<FString>(inL, 2);
	int32 Line = UTableUtil::pop<int32>(inL, 3);
	FLuaDebuggerModule::Get()->EnterDebug(LuaFilePath, Line);
	*/
}

void UScriptDebuggerSetting::SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos)
{
	SScriptDebugger::Get()->SetStackData(Content, Lines, FilePaths, StackIndex, FuncInfos);
}

FText UScriptDebuggerSetting::GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine)
{
	return SScriptDebugger::Get()->GetBreakPointHitConditionText(FilePath, CodeLine);
}

void UScriptDebuggerSetting::StepOver()
{
	if (HookingLuaState)
	{
		//LuaCall_State(HookingLuaState, "StepOver", this);
	}
}

void UScriptDebuggerSetting::StepIn()
{
	if (HookingLuaState)
	{
		//LuaCall_State(HookingLuaState, "StepIn", this);
	}
}

void UScriptDebuggerSetting::StepOut()
{
	if (HookingLuaState)
	{
		//		LuaCall_State(HookingLuaState, "StepOut", this);
	}
}

void UScriptDebuggerSetting::BreakConditionChange()
{
	//	LuaCall_AllState("BreakConditionChange", this);
}

void FScriptDebuggerVarNode::GetChildren(TArray<TSharedRef<FScriptDebuggerVarNode>>& OutChildren)
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