// Fill out your copyright notice in the Description page of Project Settings.

#include "ScriptHelperBPFunLib.h"
#include "Misc/Paths.h"
#include "LuaContext.h"
#include "UnLuaManager.h"
#include "LuaDynamicBinding.h"
#include "ScriptDataAsset.h"


bool UScriptHelperBPFunLib::GetFloatByName(UObject* Target, FName VarName, float &outFloat)
{
	if (Target) //make sure Target was set in blueprints. 
	{
		float FoundFloat;
		UFloatProperty* FloatProp = FindField<UFloatProperty>(Target->GetClass(), VarName);  // try to find float property in Target named VarName
		if (FloatProp) //if we found variable
		{
			FoundFloat = FloatProp->GetPropertyValue_InContainer(Target);  // get the value from FloatProp
			outFloat = FoundFloat;  // return float
			return true; // we can return
		}
	}
	return false; // we haven't found variable return false
}

bool UScriptHelperBPFunLib::GetScriptDataByName(UObject* Target, FName VarName, UScriptDataAsset* &outScriptData)
{
	if (Target) //make sure Target was set in blueprints. 
	{
		UScriptDataAsset* FoundScriptData = nullptr;
		UObjectProperty* ScriptProp = FindField<UObjectProperty>(Target->GetClass(), VarName);  // try to find float property in Target named VarName
		if (ScriptProp) //if we found variable
		{
			FoundScriptData = Cast<UScriptDataAsset>(ScriptProp->GetPropertyValue_InContainer(Target));  // get the value from FloatProp
			outScriptData = FoundScriptData;  // return float

			return FoundScriptData ? true:false;
		}
	}
	return false; // we haven't found variable return false
}

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
			ModuleName = InScriptData->GetDotPath();
			CodeContext.SourceCode = InScriptData->GetSourceCode();
			CodeContext.ByteCode = InScriptData->GetByteCode();
			CodeContext.Path = InScriptData->GetPath();
		}

		if (ModuleName.Len() < 1)
		{
			ModuleName = Class->GetName();
		}

		//
		GCodeContext = CodeContext;
		return Context->GetManager()->Bind(InObject, Class, *ModuleName, GLuaDynamicBinding.InitializerTableRef);
	}
	return false;
}
