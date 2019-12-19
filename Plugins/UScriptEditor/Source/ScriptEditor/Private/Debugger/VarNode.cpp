// Fill out your copyright notice in the Description page of Project Settings.

#include "VarNode.h"
#include "VarWatcher.h"
#include "UnLuaDelegates.h"
#include "lua.hpp"
#include "UnLua.h"
#include "UEReflectionUtils.h"
#include "GameFramework/Actor.h"


void FVarWatcherNode::GetChildren(TArray<TSharedRef<FVarWatcherNode>>& OutChildren)
{

}

UVarWatcherSetting* UVarWatcherSetting::Get()
{
	static UVarWatcherSetting* Singleton = NULL;
	if (Singleton == NULL)
	{
		Singleton = NewObject<UVarWatcherSetting>();
		Singleton->AddToRoot();

		//Bind UnLUa Create Lua_State delegate
		FUnLuaDelegates::OnLuaStateCreated.AddUObject(Singleton, &UVarWatcherSetting::RegisterLuaState);

		FUnLuaDelegates::OnPostLuaContextCleanup.AddUObject(Singleton, &UVarWatcherSetting::UnRegisterLuaState);

		FUnLuaDelegates::OnObjectBinded.AddUObject(Singleton, &UVarWatcherSetting::OnObjectBinded);

		FUnLuaDelegates::OnObjectUnbinded.AddUObject(Singleton, &UVarWatcherSetting::OnObjectUnbinded);
	}
	return Singleton;
}

void UVarWatcherSetting::RegisterLuaState(lua_State* State)
{
	L = State;

	//add global node
	FVarWatcherNode_Ref GlobalNode = MakeShareable(new FVarWatcherNode);
	GlobalNode->NodeType = EScriptVarNodeType::Global;
	GlobalNode->VarName = FText::FromString("Global");

	VarTreeRoot.Add(GlobalNode);
}

void UVarWatcherSetting::UnRegisterLuaState(bool bFullCleanup)
{
	L = NULL;
}

void UVarWatcherSetting::OnObjectBinded(UObjectBaseUtility* InObject)
{
	ObjectGroup.Add(InObject);

}

void UVarWatcherSetting::OnObjectUnbinded(UObjectBaseUtility* InObject)
{
	ObjectGroup.Remove(InObject);
}

void UVarWatcherSetting::Update(float DeltaTime)
{

}

