// Fill out your copyright notice in the Description page of Project Settings.

#include "ScriptHelperBPFunLib.h"
#include "Misc/Paths.h"
#include "LuaContext.h"
#include "UnLuaManager.h"
#include "LuaDynamicBinding.h"
#include "ScriptDataAsset.h"

FString UScriptHelperBPFunLib::ScriptSourceRoot()
{
	return TEXT("Script/");
}

FString UScriptHelperBPFunLib::ScriptSourceDir()
{
	return FPaths::ProjectDir() + ScriptSourceRoot();
}

bool UScriptHelperBPFunLib::TryToBindingScript(UObject* InObject, UScriptDataAsset *InScriptData)
{
	FLuaContext* Context = FLuaContext::Create();
	if (InObject &&  Context && Context->IsEnable() && Context->GetManager())
	{
		UClass *Class = InObject->GetClass();
		FString ModuleName;
		FCodeContext CodeContext;

		if (InScriptData)
		{
			//ModuleName = InScriptData->GetDotPath();
			CodeContext.Code = InScriptData->GetCode();
			CodeContext.Path = InScriptData->GetPath();
		}

		if (ModuleName.Len() < 1)
		{
			ModuleName = Class->GetName();
		}

		return Context->GetManager()->Bind(InObject, Class, *ModuleName, CodeContext, GLuaDynamicBinding.InitializerTableRef);
	}
	return false;
}
