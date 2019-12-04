// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditorToolbar.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "ScriptEditorCommands.h"
#include "LevelEditorActions.h"
#include "SourceCodeNavigation.h"


void FScriptEditorToolbar::AddEditorToolbar(TSharedPtr<FExtender> Extender)
{
	check(ScriptEditor.IsValid());
	TSharedPtr<FScriptEditor> CodeProjectEditorPtr = ScriptEditor.Pin();

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		CodeProjectEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP( this, &FScriptEditorToolbar::FillEditorToolbar ) );

	Extender->AddToolBarExtension(
		"Asset",
		EExtensionHook::Before,
		CodeProjectEditorPtr->GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FScriptEditorToolbar::FillEditToolbar));
}

void FScriptEditorToolbar::FillEditToolbar(FToolBarBuilder& ToolbarBuilder)
{
	TSharedPtr<FScriptEditor> CodeProjectEditorPtr = ScriptEditor.Pin();

	ToolbarBuilder.BeginSection(TEXT("Edit"));
	{
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().Backward);
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().Forward);
	}
	ToolbarBuilder.EndSection();

}

void FScriptEditorToolbar::FillEditorToolbar(FToolBarBuilder& ToolbarBuilder)
{
	TSharedPtr<FScriptEditor> CodeProjectEditorPtr = ScriptEditor.Pin();

	ToolbarBuilder.BeginSection(TEXT("FileManagement"));
	{
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().SaveAll);
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().Reload);
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().ReloadAll);
	}
	ToolbarBuilder.EndSection();


	// Only show the compile options on machines with the solution (assuming they can build it)
	if ( FSourceCodeNavigation::IsCompilerAvailable() )
	{
		ToolbarBuilder.BeginSection(TEXT("Build"));
		{
			struct Local
			{
				static void ExecuteCompile(TSharedPtr<FScriptEditor> InCodeProjectEditorPtr)
				{
					if(InCodeProjectEditorPtr->SaveAll())
					{
						FLevelEditorActionCallbacks::RecompileGameCode_Clicked();
					}
				}
			};

			// Since we can always add new code to the project, only hide these buttons if we haven't done so yet
			ToolbarBuilder.AddToolBarButton(
				FUIAction(
					FExecuteAction::CreateStatic(&Local::ExecuteCompile, CodeProjectEditorPtr),
					FCanExecuteAction::CreateStatic(&FLevelEditorActionCallbacks::Recompile_CanExecute),
					FIsActionChecked(),
					FIsActionButtonVisible::CreateStatic(FLevelEditorActionCallbacks::CanShowSourceCodeActions)),
				NAME_None,
				NSLOCTEXT( "LevelEditorToolBar", "CompileMenuButton", "Compile" ),
				NSLOCTEXT( "LevelEditorActions", "RecompileGameCode_ToolTip", "Recompiles and reloads C++ code for game systems on the fly" ),
				FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Recompile")
				);
		}
		ToolbarBuilder.EndSection();
		
	}

	//Debug
	ToolbarBuilder.BeginSection(TEXT("Debug"));
	{
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().DebugContinue);
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().DebugStepover);
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().DebugStepin);
		ToolbarBuilder.AddToolBarButton(FScriptEditorCommands::Get().DebugStepout);
	}
	ToolbarBuilder.EndSection();
}
