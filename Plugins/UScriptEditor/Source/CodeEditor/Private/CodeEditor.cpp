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
#include "SCodeProjectTreeEditor.h"
#include "CodeEditorUtils.h"
#include "LevelEditor.h"
#include "EditorAssetLibrary.h"

#include "Dialogs/Dialogs.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "AssetRegistryModule.h"
#include "AssetToolsModule.h"

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

	//
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnInMemoryAssetDeleted().AddRaw(this, &FCodeEditor::OnScriptAssetDeleted);
	AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FCodeEditor::OnScriptAssetRenamed);
	AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FCodeEditor::OnClearInvalidScriptAssets);

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

void FCodeEditor::OnScriptAssetDeleted(UObject*  AssetObject)
{
	/*GEditor->AddOnScreenDebugMessage(1, 1, FColor::Red, "ss");*/
	if (UScriptDataAsset* ScriptAsset = Cast<UScriptDataAsset>(AssetObject))
	{
		//Check file is exist and delete it
		if (IFileManager::Get().FileExists(*ScriptAsset->Path))
		{
			if (IFileManager::Get().Delete(*ScriptAsset->Path))
			{
				//Log success
				GEditor->AddOnScreenDebugMessage(1, 1, FColor::Red, "Deleted file succes:" + ScriptAsset->Path);
				TSharedPtr<FCodeProjectEditor> ProjectEditor = FCodeProjectEditor::Get();
				if (ProjectEditor.IsValid())
				{
					
					if (UCodeProjectItem* Item = Cast<UCodeProjectItem>(ScriptAsset->UserObject))
					{
						if (Item->Parent)Item->Parent->Children.Remove(Item);
						ProjectEditor->CloseEditingFile(Item);

						TSharedPtr<SCodeProjectTreeEditor> ProjectTreeEditor = SCodeProjectTreeEditor::Get();
						if (ProjectTreeEditor.IsValid())
						{
							ProjectTreeEditor->RequestRefresh();
						}
					}
					else
					{						
						//Reflash Browser
						ProjectEditor->RescanScriptProject();				
					}
				}
			}
			else
			{
				//Log fail
				GEditor->AddOnScreenDebugMessage(1, 1, FColor::Red, "Deleted file failed:" + ScriptAsset->Path);
			}
		}
		else
		{
			//Log no file
			GEditor->AddOnScreenDebugMessage(1, 1, FColor::Red, "No file:" + ScriptAsset->Path);
		}
	}
	else
	{
		//
		GEditor->AddOnScreenDebugMessage(1, 1, FColor::Red, "Error ScriptAsset file");
	}

}

void FCodeEditor::OnScriptAssetRenamed(const FAssetData& RenamedAsset, const FString& OldObjectPath)
{
	GEditor->AddOnScreenDebugMessage(1, 1, FColor::Red, "rename:" + RenamedAsset.AssetName.ToString());

	if (UScriptDataAsset* ScriptAsset = Cast<UScriptDataAsset>(RenamedAsset.FastGetAsset()))
	{
		if (IFileManager::Get().FileExists(*ScriptAsset->Path))
		{
			//create new script file
			FString NewScriptPath = FPaths::GetPath(ScriptAsset->Path) + "/" + ScriptAsset->GetName() + FPaths::GetExtension(ScriptAsset->Path, true);
			FFileHelper::SaveStringToFile(ScriptAsset->CodeText, *NewScriptPath);
			//delete old script file
			IFileManager::Get().Delete(*ScriptAsset->Path);
			//
			ScriptAsset->Path = NewScriptPath;

			TSharedPtr<FCodeProjectEditor> ProjectEditor = FCodeProjectEditor::Get();

			//change the item info
			if (UCodeProjectItem* Item = Cast<UCodeProjectItem>(ScriptAsset->UserObject))
			{
				Item->Path = NewScriptPath;
				Item->Name = ScriptAsset->GetName();
				//TODO:Refresh Project
				TSharedPtr<SCodeProjectTreeEditor> ProjectTreeEditor = SCodeProjectTreeEditor::Get();
				if (ProjectTreeEditor.IsValid())
				{
					/************************************************************************/
					//TODO:Bad code,should refresh item         				
					ProjectTreeEditor->ExpanedScriptItem(Item,false);
					if (ProjectEditor.IsValid())
					{
						ProjectEditor->CloseEditingFile(Item);
					}
					/************************************************************************/
				}
			}
			else
			{
				//Reflash Browser
				if (ProjectEditor.IsValid())
				{
					ProjectEditor->RescanScriptProject();
				}
			}

		}
		else
		{
			//Log error
		}
	}

	//Test
	//OnClearInvalidScriptAssets();
}

void FCodeEditor::OnClearInvalidScriptAssets()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDatas;
	TArray<FAssetData> InvalidAssetDatas;
	FARFilter Filter;

	Filter.ClassNames.Add(UScriptDataAsset::StaticClass()->GetFName());
	Filter.PackagePaths.Add("/Game");//Set the script path to optimization
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDatas);

	for (FAssetData ScriptAsset:AssetDatas)
	{
		//////////////////////////////////////////////////////////////////////////
		//Fast Check,but Unsafe
		FString ScriptPath = CodeEditorUtils::CovertAssetPathToContentPath(ScriptAsset.GetFullName());
		if (ScriptAsset.GetClass()->IsChildOf(ULuaScript::StaticClass()))
		{
			ScriptPath += ".lua";
		}
		//js....
		if (!FPaths::FileExists(ScriptPath))
		{
			if (UScriptDataAsset* UScript = Cast<UScriptDataAsset>(ScriptAsset.FastGetAsset(true)))
			{
				UScript->EFlag_IsValid = false;
			}
			InvalidAssetDatas.Add(ScriptAsset);
		}
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		//Slowly check,but safe
		/*
		if (UScriptDataAsset* UScript = Cast<UScriptDataAsset>(ScriptAsset.FastGetAsset(true)))
		{
			UScript->EFlag_IsValid = FPaths::FileExists(UScript->Path);
			if (!UScript->EFlag_IsValid)
			{
				InvalidAssetDatas.Add(ScriptAsset);
			}
		}
		*/
		//////////////////////////////////////////////////////////////////////////
	}
	//Show a Dialog to delete invalid UScriptAssets
	if (InvalidAssetDatas.Num() > 0)
	{
		FText WaringTips = LOCTEXT("InvalidScriptAssetWaring", "InvalidAsset");
		EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("DeleteInvalidScriptAsset", "Do you want to delete the Invalid ScriptAsset..."), &WaringTips);

		if (ReturnType == EAppReturnType::Yes)
		{
			for (FAssetData DetetedAsset:InvalidAssetDatas)
			{
				//AssetRegistryModule.AssetDeleted(DetetedAsset.FastGetAsset(true));
				UEditorAssetLibrary::DeleteAsset(DetetedAsset.GetFullName());
			}
		}
	}

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
