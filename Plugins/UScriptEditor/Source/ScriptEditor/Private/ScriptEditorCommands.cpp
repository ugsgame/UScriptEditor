// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditorCommands.h"
#include "ScriptEditorStyle.h"


#define LOCTEXT_NAMESPACE "ScriptEditorCommands"


FScriptEditorCommands::FScriptEditorCommands() 
	: TCommands<FScriptEditorCommands>("ScriptEditor", LOCTEXT("General", "General"), NAME_None, FScriptEditorStyle::GetStyleSetName())
{
}


void FScriptEditorCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "UScriptEditor", "Bring up UScriptEditor", EUserInterfaceActionType::Button, FInputGesture());

	UI_COMMAND(Reload, "Reload", "Reload the currently active document file.", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ReloadAll, "Reload All", "Reload all document file.", EUserInterfaceActionType::Button, FInputChord());

	UI_COMMAND(Save, "Save", "Save the currently active document.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::S));
	UI_COMMAND(SaveAll, "Save All", "Save all open documents.", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::S));

	UI_COMMAND(Backward, "Backward", "Backward", EUserInterfaceActionType::Button, FInputChord(EKeys::LeftAlt));
	UI_COMMAND(Forward, "Forward", "Forward", EUserInterfaceActionType::Button, FInputChord(EKeys::RightAlt));

	UI_COMMAND(DebugContinue, "Continue", "Continue", EUserInterfaceActionType::Button, FInputChord(EKeys::F5));
	UI_COMMAND(DebugStepover, "Step Over", "Step Over", EUserInterfaceActionType::Button, FInputChord(EKeys::F10));
	UI_COMMAND(DebugStepin, "Step In", "Step In", EUserInterfaceActionType::Button, FInputChord(EKeys::F11));
	UI_COMMAND(DebugStepout, "Step Out", "Step Out", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::F11));

	UI_COMMAND(APIBroswer, "APIBroswer", "APIBroswer", EUserInterfaceActionType::Button, FInputGesture());
}


#undef LOCTEXT_NAMESPACE
