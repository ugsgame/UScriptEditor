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

public:

	struct lua_State *L;

	TArray<UObjectBaseUtility*> ObjectGroup;

	TArray<TSharedRef<FVarWatcherNode>> VarTreeRoot;

};

