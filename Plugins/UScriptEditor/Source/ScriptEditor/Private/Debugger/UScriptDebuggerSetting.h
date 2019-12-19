// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "STreeView.h"
//#include "LuaDelegateMulti.h"

#include "UScriptDebuggerSetting.generated.h"



UENUM()
enum class EScriptVarNodeType : uint8
{
	Local = 0,
	UpValue,
	Global,
	UEObject
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

USTRUCT()
struct FScriptDebuggerVarNode
{
	GENERATED_BODY()

public:

	FScriptDebuggerVarNode() {}

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
		FString NameListString;
		for (auto NameStr : NameList)
		{
			NameListString += FString("-->").Append(NameStr);
		}
		return FString::Printf(TEXT("unlua_Debug VarNode VarName[%s] NameList[%s]"), *VarName.ToString(), *NameListString);
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

	bool bIsStart;
	bool bTabIsOpen;
	struct lua_State *L;

	class FClassDesc* UEClassDesc;
	UObject* UEObject;
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

public:

	UScriptDebuggerSetting() :L(nullptr) {}

	UFUNCTION()
		static UScriptDebuggerSetting* Get();

	virtual void SetTabIsOpen(bool IsOpen);

	UPROPERTY(config)
		TMap<FString, float> LastTimeFileOffset;

	UPROPERTY(config)
		FString RecentFilePath;

	UPROPERTY(config)
		TArray<FScriptBreakPointNode> RecentBreakPoint;

	//UPROPERTY(config)
	TMap<FString, TMap<FString, FScriptPromptNode>> ScriptPromptGroup;

	TArray<FScriptPromptNode> ScriptPromptArray;

	UPROPERTY()
		FScriptBreakPointNode HittingPoint;

	void OnObjectBinded(UObjectBaseUtility* InObject);

	void RegisterLuaState(lua_State* State);

	virtual void Continue();

	virtual void StepOver();

	virtual void StepIn();

	virtual void StepOut();

	void UnRegisterLuaState(bool bFullCleanup);

	bool NameTranslate(int32 KindType, FString& VarName, int32 StackIndex);

	void ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex);

	void IteraionTable(FScriptDebuggerVarNode& InNode, int32 NameIndex);

	void LocalListen(FScriptDebuggerVarNode& InNode);

	void UpvalueListen(FScriptDebuggerVarNode& InNode);

	void GlobalListen(FScriptDebuggerVarNode& InNode);

	bool PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, UProperty* Property, UObject* Object);

	void UEObjectListen(FScriptDebuggerVarNode& InNode);

	void EnterDebug(const FString& LuaFilePath, int32 Line);

	virtual void GetStackVars(int32 StackIndex, TArray<FScriptDebuggerVarNode_Ref>& Vars);

	virtual void GetVarsChildren(FScriptDebuggerVarNode& Node);
};
