﻿// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "LuaScriptAssetTypeActions.h"
#include "CodeEditor.h"
#include "LuaWrapper/LuaScript.h"
#include "CodeProject.h"
#include "CodeProjectEditor.h"
#include "SCodeProjectTreeEditor.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "LuaScriptAssetTypeActions"


//////////////////////////////////////////////////////////////////////////
// FAssetTypeAction_Quest

FText FLuaScriptAssetTypeActions::GetName() const
{
	return LOCTEXT("FLuaScriptAssetTypeActionsName", "Lua");
}

FColor FLuaScriptAssetTypeActions::GetTypeColor() const
{
	return FColor(244, 145, 65);
}

UClass* FLuaScriptAssetTypeActions::GetSupportedClass() const
{
	return ULuaScript::StaticClass();
}

void FLuaScriptAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	if (UScriptDataAsset* ScriptAsset = Cast<UScriptDataAsset>(InObjects[0]))
	{
		FGlobalTabmanager::Get()->InvokeTab(FCodeEditor::CodeEditorTabName);

		TSharedPtr<FCodeProjectEditor> ProjectEditor = FCodeProjectEditor::Get();
		if (ProjectEditor.IsValid())
		{
			if (UCodeProjectItem* Item = Cast<UCodeProjectItem>(ScriptAsset->UserObject))
			{
				//Goto item tab
				FCodeProjectEditor::Get()->OpenFileForEditing(Item);
				//Expaned this item
				if (SCodeProjectTreeEditor::Get().IsValid())
				{
					SCodeProjectTreeEditor::Get()->ExpanedScriptItem(Item);
				}
				//
			}
			else
			{
				//Check file is exist
				uint32 Flags = 0;
				FArchive* Reader = IFileManager::Get().CreateFileReader(*ScriptAsset->Path, Flags);
				if (Reader && Flags > 0)
				{

				}
				// Unlegalized Asset File!!
				else
				{

				}
			}
		}

	}
}

uint32 FLuaScriptAssetTypeActions::GetCategories()
{
	if (FCodeEditor* Module = FCodeEditor::GetInstance()) {
		return Module->GetAssetCategoryBit();
	}
	// If Narrative Extension module is not found use Miscellaneous 
	return EAssetTypeCategories::Misc;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE