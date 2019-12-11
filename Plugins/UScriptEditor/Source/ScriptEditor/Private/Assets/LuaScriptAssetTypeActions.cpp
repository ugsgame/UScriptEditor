// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "LuaScriptAssetTypeActions.h"
#include "ScriptEditorModule.h"
#include "LuaWrapper/LuaScript.h"
#include "SourceProject.h"
#include "ScriptEditor.h"
#include "SProjectTreeEditor.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "ScriptEditorSetting.h"

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
		if (ScriptAsset->EFlag_IsValid)
		{
			//FGlobalTabmanager::Get()->InvokeTab(FScriptEditorModule::ScriptEditorTabName);
			FScriptEditorModule::GetInstance()->OpenEditorWindow();

			TSharedPtr<FScriptEditor> ProjectEditor = FScriptEditor::Get();
			if (ProjectEditor.IsValid())
			{
				if (UCodeProjectItem* Item = Cast<UCodeProjectItem>(ScriptAsset->UserObject))
				{
					//Goto item tab			
					FScriptEditor::Get()->OpenFileForEditing(Item);

					UScriptEdtiorSetting::Get()->EdittingFiles.Remove(Item->Path);	//remove if exist
					UScriptEdtiorSetting::Get()->EdittingFiles.Add(Item->Path);		// add to last!
					//Expaned this item
					if (SProjectTreeEditor::Get().IsValid())
					{
						SProjectTreeEditor::Get()->ExpanedScriptItem(Item);
					}
					//
				}
				else
				{
					////Show Waring Dialog
				}
			}
			else
			{
				//TODO:Add the item to open after ProjectEditor was inited!!!
			}
		}
		else
		{
			//Show Waring Dialog
		}
	}
}

uint32 FLuaScriptAssetTypeActions::GetCategories()
{
	if (FScriptEditorModule* Module = FScriptEditorModule::GetInstance()) {
		return Module->GetAssetCategoryBit();
	}
	// If Narrative Extension module is not found use Miscellaneous 
	return EAssetTypeCategories::Misc;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE