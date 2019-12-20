// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UScriptDebuggerSetting.h"
#include "VarNode.generated.h"

struct lua_State;

struct FVarWatcherNode
{
	EScriptVarNodeType NodeType;

	int32 KindType;

	FText VarName;

	FText VarValue;

	FText VarType;

	void* VarPtr;

	TArray<FString> NameList;

	TMap<FString, TSharedRef<FVarWatcherNode>> NodeChildren;

	void GetChildren(TArray<TSharedRef<FVarWatcherNode>>& OutChildren);
};

using FVarWatcherNode_Ref = TSharedRef<FVarWatcherNode>;
using SVarWatcherTree = STreeView<FVarWatcherNode_Ref>;

UCLASS()
class  UVarWatcherSetting : public UObject
{
	GENERATED_BODY()

public:

	static UVarWatcherSetting* Get();

	void RegisterLuaState(lua_State* State);

	void UnRegisterLuaState(bool bFullCleanup);

	void OnObjectBinded(UObjectBaseUtility* InObject);

	void OnObjectUnbinded(UObjectBaseUtility* InObject);

	void Update(float DeltaTime);

	void GetVarChildren(FVarWatcherNode& InNode);

public:

	struct lua_State *L;

	TArray<UObjectBaseUtility*> ObjectGroup;

	TArray<TSharedRef<FVarWatcherNode>> VarTreeRoot;

private:

	void IteraionTable(FVarWatcherNode& InNode, int32 NameIndex);

	bool NameTranslate(int32 KindType, FString& VarName, int32 StackIndex);

	void ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex);

	void GlobalListen(FVarWatcherNode& InNode);

	bool PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, UProperty* Property, UObject* Object);

	void UEObjectListen(FVarWatcherNode& InNode);

private:

	static const FString TempVarName;
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

};

