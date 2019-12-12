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
#include "ScriptHelperBPFunLib.h"

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

			if (FScriptEditorModule::GetInstance()->IsEditorOpen)
			{
				//Goto item tab			
				UCodeProjectItem* Item = Cast<UCodeProjectItem>(ScriptAsset->UserObject);
				TSharedPtr<FScriptEditor> ScriptEditor = FScriptEditor::Get();
				if (ScriptEditor.IsValid() && Item)
				{
					FScriptEditor::Get()->OpenFileForEditing(Item);

					//Expaned this item
					if (SProjectTreeEditor::Get().IsValid())
					{
						SProjectTreeEditor::Get()->ExpanedScriptItem(Item);
					}
				}
			}
			else
			{
				FString ScriptPath = UScriptHelperBPFunLib::ScriptSourceDir()  + ScriptAsset->Path;
				//remove if exist
				UScriptEdtiorSetting::Get()->EdittingFiles.Remove(ScriptPath);	
				// add to last!
				UScriptEdtiorSetting::Get()->EdittingFiles.Add(ScriptPath);

				FScriptEditorModule::GetInstance()->OpenEditorWindow();
			}
		}
		else
		{
			//TODO:Show a dialog
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