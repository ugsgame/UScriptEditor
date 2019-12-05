// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "STreeView.h"
//#include "LuaDelegateMulti.h"
#include "UScriptDebuggerSetting.generated.h"

USTRUCT()
struct  FScriptDebuggerVarNode
{
	GENERATED_BODY()

		FScriptDebuggerVarNode() :
		NameWeakIndex(-1), ValueWeakIndex(-1)
	{}

	UPROPERTY()
		FText Name;

	UPROPERTY()
		FText Value;

	UPROPERTY()
		int32 NameWeakIndex;

	UPROPERTY()
		int32 ValueWeakIndex;

	class UScriptDebuggerSetting* DebuggerSetting;

	void GetChildren(TArray<TSharedRef<FScriptDebuggerVarNode>>& OutChildren);

	TArray<TSharedRef<FScriptDebuggerVarNode>> KeyChildren;
	TArray<TSharedRef<FScriptDebuggerVarNode>> ValueChildren;
};

using FDebuggerVarNode_Ref = TSharedRef<FScriptDebuggerVarNode>;
using SDebuggerVarTree = STreeView<FDebuggerVarNode_Ref>;

USTRUCT()
struct  FScriptBreakPointNode
{
	GENERATED_BODY()
		FScriptBreakPointNode() {};
	FScriptBreakPointNode(int32 _Line, FString _FilePath)
		:FilePath(_FilePath), Line(_Line)
	{}
	UPROPERTY()
		FString FilePath;

	UPROPERTY()
		int32 Line;

	UPROPERTY()
		FText HitCondition;
};

UCLASS(config = Editor)
class  UScriptDebuggerSetting : public UObject
{
	GENERATED_BODY()
		TMap<FString, TSet<int32>> BreakPoints;
	bool bIsStart;
	bool bTabIsOpen;
	struct lua_State *HookingLuaState;
public:
	UScriptDebuggerSetting() :HookingLuaState(nullptr) {}
	UFUNCTION()
		static UScriptDebuggerSetting* Get(bool IsRemoteDebugger = false);
	UFUNCTION()
		virtual FString GetLuaSourceDir();

	UFUNCTION()
		virtual void PullDataToLua();
	virtual void UpdateBreakPoint(TMap<FString, TSet<int32>>& BreakPoint);
	virtual void ToggleDebugStart(bool IsStart);
	virtual void SetTabIsOpen(bool IsOpen);
	virtual void GetStackVars(int32 StackIndex, TArray<FDebuggerVarNode_Ref>& Vars);
	virtual void GetVarsChildren(FScriptDebuggerVarNode Node, TArray<FDebuggerVarNode_Ref>& OutChildren);


	virtual void EnterDebug(lua_State* State);

	UFUNCTION()
		virtual void SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos);

	UFUNCTION()
		virtual FText GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine);

	void CombineNodeArr(TArray<FDebuggerVarNode_Ref>& PreVars, TArray<FScriptDebuggerVarNode>& NowVars);
	virtual void DebugContinue() {};
	virtual void StepOver();
	virtual void StepIn();
	virtual void StepOut();
	virtual void BreakConditionChange();

	UPROPERTY(config)
	TMap<FString, float> LastTimeFileOffset;

	UPROPERTY(config)
	FString RecentFilePath;

	UPROPERTY(config)
	TArray<FScriptBreakPointNode> RecentBreakPoint;
};
