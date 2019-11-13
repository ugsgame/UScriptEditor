// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaScriptFactory.h"
#include "CodeEditor.h"
#include "AssetToolsModule.h"
#include "LuaWrapper/LuaScript.h"

#define LOCTEXT_NAMESPACE "LuaScriptFactory" 

ULuaScriptFactory::ULuaScriptFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = ULuaScript::StaticClass();
}

UObject* ULuaScriptFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class == ULuaScript::StaticClass() || Class->IsChildOf(ULuaScript::StaticClass()));
	return NewObject<ULuaScript>(InParent, Class, Name, Flags);
}

// uint32 ULuaScriptFactory::GetMenuCategories() const
// {
// 	if (FCodeEditor* Module = FCodeEditor::GetInstance()) {
// 		return Module->GetAssetCategoryBit();
// 	}
// 	return EAssetTypeCategories::Misc;
// }
// 
FText ULuaScriptFactory::GetDisplayName() const
{
	return LOCTEXT("LuaScriptText", "Lua");
}

FString ULuaScriptFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("NewLua"));
}

#undef LOCTEXT_NAMESPACE