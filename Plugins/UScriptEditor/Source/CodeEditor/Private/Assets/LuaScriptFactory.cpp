// Fill out your copyright notice in the Description page of Project Settings.


#include "LuaScriptFactory.h"
#include "CodeEditor.h"
#include "CodeEditorUtils.h"
#include "AssetToolsModule.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "LuaWrapper/LuaScript.h"

#define LOCTEXT_NAMESPACE "LuaScriptFactory" 

ULuaScriptFactory::ULuaScriptFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = ULuaScript::StaticClass();

	//Formats.Add("lua;Script");

	bCreateNew = true;
	//bEditorImport = true;
	bEditAfterNew = true;
}

UObject* ULuaScriptFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(InClass == ULuaScript::StaticClass() || InClass->IsChildOf(ULuaScript::StaticClass()));
	ULuaScript* ScriptAsset = NewObject<ULuaScript>(InParent, InClass, InName, Flags);
	//create *.lua in this directory
	CodeEditorUtils::CreateLuaFileFromLuaScriptAsset(ScriptAsset);
	//save asset
	CodeEditorUtils::SaveScriptAsset(ScriptAsset);

	//Reflash Browser
	TSharedPtr<FCodeProjectEditor> ProjectEditor = FCodeProjectEditor::Get();
	if (ProjectEditor.IsValid())
	{
		ProjectEditor->GetScriptProjectBeingEdited()->Children.Empty();
		ProjectEditor->GetScriptProjectBeingEdited()->RescanChildren();
	}
	else
	{
		//Open Editor
		FGlobalTabmanager::Get()->InvokeTab(FCodeEditor::CodeEditorTabName);
	}

	return ScriptAsset;
}

/*
UObject* ULuaScriptFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	GEditor->SelectNone(true, true, false);

	check(InClass == ULuaScript::StaticClass() || InClass->IsChildOf(ULuaScript::StaticClass()));
	ULuaScript* ScriptAsset = NewObject<ULuaScript>(InParent, InClass, InName, Flags);
	//
	FString CodeText;
	if (FFileHelper::LoadFileToString(CodeText, *Filename))
	{
		ScriptAsset->CodeText = CodeText;
	}

	//save asset
	CodeEditorUtils::SaveScriptAsset(ScriptAsset);
	//save lua file in same directory


	return ScriptAsset;
}

UObject* ULuaScriptFactory::FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)
{
	GEditor->SelectNone(true, true, false);

	check(InClass == ULuaScript::StaticClass() || InClass->IsChildOf(ULuaScript::StaticClass()));
	ULuaScript* ScriptAsset = NewObject<ULuaScript>(InParent, InClass, InName, Flags);

	//save asset
	CodeEditorUtils::SaveScriptAsset(ScriptAsset);

	return ScriptAsset;
}

bool ULuaScriptFactory::FactoryCanImport(const FString& Filename)
{
	if (FPaths::GetExtension(Filename) == "lua")
	{
		bCreateNew = false;
		return true;
	}
	bCreateNew = true;
	return false;
}
*/
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