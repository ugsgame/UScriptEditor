// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "LuaScriptAssetTypeActions.h"
#include "CodeEditor.h"
#include "LuaWrapper/LuaScript.h"
#include "CodeProject.h"
#include "CodeProjectEditor.h"
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
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (UCodeProject* CodeProject = Cast<UCodeProject>(*ObjIt))
		{
			TSharedRef<FCodeProjectEditor> NewCodeProjectEditor(new FCodeProjectEditor());
			NewCodeProjectEditor->InitCodeEditor(Mode, EditWithinLevelEditor, CodeProject);
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