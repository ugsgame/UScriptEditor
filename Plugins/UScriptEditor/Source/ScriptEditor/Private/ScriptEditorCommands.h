// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

class FScriptEditorCommands : public TCommands<FScriptEditorCommands>
{
public:
	FScriptEditorCommands();

	TSharedPtr<FUICommandInfo> Save;
	TSharedPtr<FUICommandInfo> SaveAll;
	TSharedPtr< FUICommandInfo > OpenPluginWindow;

	//////////////////////////////////////////////////////////////////////////
	//Debug Commands       
	TSharedPtr<FUICommandInfo> DebugContinue;
	TSharedPtr<FUICommandInfo> DebugStepover;
	TSharedPtr<FUICommandInfo> DebugStepin;
	TSharedPtr<FUICommandInfo> DebugStepout;
	//////////////////////////////////////////////////////////////////////////

	/** Initialize commands */
	virtual void RegisterCommands() override;

};
