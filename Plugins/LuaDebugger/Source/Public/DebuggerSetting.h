// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "STreeView.h"
//#include "LuaDelegateMulti.h"

#include "DebuggerSetting.generated.h"


UENUM()
enum class EVarNodeType : uint8
{
	Local = 0,
	UpValue,
	Global,
	UEObject
};

USTRUCT()
struct LUADEBUGGER_API FDebuggerVarNode
{
	GENERATED_BODY()

public:

	FDebuggerVarNode() {}

	UPROPERTY()
		EVarNodeType NodeType;

	UPROPERTY()
		int32 StackLevel;

	UPROPERTY()
		int32 VarType;

	UPROPERTY()
		FText VarName;

	UPROPERTY()
		FText VarValue;

	UPROPERTY()
		TArray<FString> NameList;

	TMap<FString, TSharedRef<FDebuggerVarNode>> NodeChildren;

	void GetChildren(TArray<TSharedRef<FDebuggerVarNode>>& OutChildren);

	bool IsEditable();

	FString ToString()
	{
		FString NameListString;
		for (auto NameStr : NameList)
		{
			NameListString += FString("-->").Append(NameStr);
		}
		return FString::Printf(TEXT("unlua_Debug VarNode VarName[%s] NameList[%s]"), *VarName.ToString(), *NameListString);
	}

};

using FDebuggerVarNode_Ref = TSharedRef<FDebuggerVarNode>;
using SDebuggerVarTree = STreeView<FDebuggerVarNode_Ref>;

USTRUCT()
struct LUADEBUGGER_API FBreakPointNode
{
	GENERATED_BODY()
		FBreakPointNode() {};
	FBreakPointNode(int32 _Line, FString _FilePath)
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
class LUADEBUGGER_API UDebuggerSetting : public UObject
{
	GENERATED_BODY()
public:

	TMap<FString, TSet<int32>> BreakPoints;
	bool bIsStart;
	bool bTabIsOpen;
	struct lua_State *L;
public:
	UDebuggerSetting() :L(nullptr) {}
	UFUNCTION()
		static UDebuggerSetting* Get(bool IsRemoteDebugger = false);
	UFUNCTION()
		virtual FString GetLuaSourceDir();

	UFUNCTION()
		virtual void PullDataToLua();
	virtual void UpdateBreakPoint(TMap<FString, TSet<int32>>& BreakPoint);
	virtual void ToggleDebugStart(bool IsStart);
	virtual void SetTabIsOpen(bool IsOpen);



	UFUNCTION()
		virtual void SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos);

	UFUNCTION()
		virtual FText GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine);

	void CombineNodeArr(TArray<FDebuggerVarNode_Ref>& PreVars, TArray<FDebuggerVarNode>& NowVars);

	virtual void BreakConditionChange();
	UPROPERTY(config)
		TMap<FString, float> LastTimeFileOffset;

	UPROPERTY(config)
		FString RecentFilePath;

	UPROPERTY(config)
		TArray<FBreakPointNode> RecentBreakPoint;

	void RegisterLuaState(lua_State* State);
	virtual void Continue();
	virtual void StepOver();
	virtual void StepIn();
	virtual void StepOut();
	void UnRegisterLuaState(bool bFullCleanup);

	bool NameTranslate(int32 VarType, FString& VarName, int32 StackIndex);

	void ValueTranslate(int32 VarType, FString& VarValue, int32 StackIndex);

	void IteraionTable(FDebuggerVarNode& InNode, int32 NameIndex);

	void LocalListen(FDebuggerVarNode& InNode);

	void UpvalueListen(FDebuggerVarNode& InNode);

	void GlobalListen(FDebuggerVarNode& InNode);

	void UEObjectListen();

	void EnterDebug(const FString& LuaFilePath, int32 Line);

	virtual void GetStackVars(int32 StackIndex, TArray<FDebuggerVarNode_Ref>& Vars);

	virtual void GetVarsChildren(FDebuggerVarNode& Node);


};
