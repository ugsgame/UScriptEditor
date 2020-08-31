// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ScriptHookType.h"

// UE 4.25 Compatible.
#include "UnLua/Private/UnLuaCompatibility.h"

#include "UScriptDebuggerSetting.generated.h"


USTRUCT()
struct FScriptPromptNode
{
	GENERATED_BODY()

	FScriptPromptNode() {}

	FScriptPromptNode(FString InCategory, FString InMenuDesc, FString InToolTip, FString InCodeClip) : Category(InCategory), MenuDesc(InMenuDesc), ToolTip(InToolTip), CodeClip(InCodeClip) {}

	UPROPERTY()
		FString Category;

	UPROPERTY()
		FString MenuDesc;

	UPROPERTY()
		FString ToolTip;

	UPROPERTY()
		FString CodeClip;

	FString ToString()
	{
		return FString::Printf(TEXT("PromptNode Category[%s] MenuDesc[%s] ToolTip[%s] CodeClip[%s] "), *Category, *MenuDesc, *ToolTip, *CodeClip);
	}

};

UCLASS(config = Editor)
class  UScriptDebuggerSetting : public UObject
{
	GENERATED_BODY()

public:

	bool bTabIsOpen;
	lua_State *L;

	unlua_over u_over;

	unlua_out u_out;

	EHookMode hook_mode;

	class FClassDesc* UEClassDesc;
	UObject* UEObject;
	static const FString TempVarName;
	static const FString SelfName;
	static const FString OverriddenName;
	static const FText ClassDescText;
	static const FText UFunctionText;

	//transform name
	static const FString SelfLocationName;
	static const FString SelfRotatorName;
	static const FString SelfScalerName;
	static const FString FVectorName;
	static const FString FRotatorName;
	static const FString ReturnValueName;

	//transform type
	static const FText SelfLocationText;
	static const FText SelfRotatorText;
	static const FText SelfScalerText;
	static const FText FVectorText;
	static const FText FRotatorText;

	FDelegateHandle RegLuaHandle;
	FDelegateHandle UnRegLuaHandle;

public:

	UScriptDebuggerSetting() :L(nullptr) {}

	UFUNCTION()
		static UScriptDebuggerSetting* Get();

	void SetTabIsOpen(bool IsOpen);

	UPROPERTY(config)
		TMap<FString, float> LastTimeFileOffset;

	UPROPERTY(config)
		FString RecentFilePath;

	UPROPERTY(config)
		TArray<FScriptBreakPointNode> RecentBreakPoint;

	TMap<FString, TMap<FString, FScriptPromptNode>> ScriptPromptGroup;

	TArray<FScriptPromptNode> ScriptPromptArray;

	UPROPERTY()
		FScriptBreakPointNode HittingPoint;

	void BindDebugState();

	void UnBindDebugState();

	void OnObjectBinded(UObjectBaseUtility* InObject);

	void RegisterLuaState(lua_State* State);

	void Continue();

	void StepOver();

	void StepIn();

	void StepOut();

	void UnRegisterLuaState(bool bFullCleanup);

	void hook_call_option();

	void hook_ret_option();

	void hook_line_option();

	bool NameTranslate(int32 KindType, FString& VarName, int32 StackIndex);

	void ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex);

	void IteraionTable(FScriptDebuggerVarNode& InNode, int32 NameIndex);

	void LocalListen(FScriptDebuggerVarNode& InNode);

	void UpvalueListen(FScriptDebuggerVarNode& InNode);

	void GlobalListen(FScriptDebuggerVarNode& InNode);

	bool PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, FProperty* Property, UObject* Object);

	void UEObjectListen(FScriptDebuggerVarNode& InNode);

	void EnterDebug(const FString& LuaFilePath, int32 Line);

	void GetStackVars(int32 StackIndex, TArray<FScriptDebuggerVarNode_Ref>& Vars);

	void GetVarsChildren(FScriptDebuggerVarNode& InNode);
};
