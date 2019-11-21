// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#include "CodeEditor.h"
#include "Modules/ModuleInterface.h"
#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "Toolkits/IToolkitHost.h"
#include "CodeEditorStyle.h"
#include "CodeProjectEditorCommands.h"
#include "CodeProject.h"
#include "ScriptProject.h"
#include "CodeProjectEditor.h"
#include "CodeEditorUtils.h"
#include "LevelEditor.h"

#include "Assets/LuaScriptAssetTypeActions.h"

#define LOCTEXT_NAMESPACE "CodeEditor"

const FName FCodeEditor::CodeEditorTabName(TEXT("CodeEditor"));

/** IModuleInterface implementation */
void FCodeEditor::StartupModule()
{
	FCodeEditorStyle::Initialize();
	FCodeEditorStyle::ReloadTextures();

	FCodeProjectEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FCodeProjectEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FCodeEditor::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension(
			"WindowLayout", 
			EExtensionHook::After,
			PluginCommands,
			FMenuExtensionDelegate::CreateRaw(this, &FCodeEditor::AddMenuExtension)
		);

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"Settings",
			EExtensionHook::After,
			PluginCommands,
			FToolBarExtensionDelegate::CreateRaw(this, &FCodeEditor::AddToolbarExtension)
		);

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(CodeEditorTabName, FOnSpawnTab::CreateRaw(this, &FCodeEditor::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("CodeEditorTabTitle", "UScriptEditor"))
		.SetTooltipText(LOCTEXT("CodeEditorTooltipText", "Open the Code Editor tab."))
		.SetIcon(FSlateIcon(FCodeEditorStyle::Get().GetStyleSetName(), "CodeEditor.TabIcon"));
		//.SetMenuType(ETabSpawnerMenuType::Hidden);


	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	RegisterAssetTypeAction(AssetTools, MakeShareable(new FLuaScriptAssetTypeActions));
	//...
	// register JK category so that assets can register to it
	AssetCategoryBit = AssetTools.RegisterAdvancedAssetCategory(FName(TEXT("UScript")), LOCTEXT("UScriptAssetCategory", "UScript"));

}

void FCodeEditor::ShutdownModule()
{
	FCodeEditorStyle::Shutdown();

	FCodeProjectEditorCommands::Unregister();

	// Unregister the tab spawner
	FGlobalTabmanager::Get()->UnregisterTabSpawner(CodeEditorTabName);
}

TSharedRef<SDockTab> FCodeEditor::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<FCodeProjectEditor> NewCodeProjectEditor = MakeShareable(new FCodeProjectEditor());
	NewCodeProjectEditor->InitCodeEditor(EToolkitMode::Standalone, TSharedPtr<class IToolkitHost>(), GetMutableDefault<UCodeProject>(), GetMutableDefault<UScriptProject>());
	
	return FGlobalTabmanager::Get()->GetMajorTabForTabManager(NewCodeProjectEditor->GetTabManager().ToSharedRef()).ToSharedRef();
}

void FCodeEditor::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(CodeEditorTabName);
}

void FCodeEditor::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FCodeProjectEditorCommands::Get().OpenPluginWindow);
}

void FCodeEditor::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FCodeProjectEditorCommands::Get().OpenPluginWindow);

// 	Builder.AddToolBarButton(
// 		FCodeProjectEditorCommands::Get().OpenPluginWindow,
// 		"UScriptEditor",
// 		LOCTEXT("CodeEditorTabTitle", "Edit Source Code"),
// 		LOCTEXT("CodeEditorTooltipText", "Open the Code Editor tab."),
// 		FSlateIcon(FCodeEditorStyle::Get().GetStyleSetName(), "CodeEditor.TabIcon")
// 	);
}


IMPLEMENT_MODULE(FCodeEditor, CodeEditor)

#undef LOCTEXT_NAMESPACE
