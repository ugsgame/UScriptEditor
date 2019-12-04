// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScriptEditor.h"

class FToolBarBuilder;

class FScriptEditorToolbar : public TSharedFromThis<FScriptEditorToolbar>
{
public:
	FScriptEditorToolbar(TSharedPtr<class FScriptEditor> InScriptEditor)
		: ScriptEditor(InScriptEditor) {}

	void AddEditorToolbar(TSharedPtr<FExtender> Extender);

private:

	void FillEditToolbar(FToolBarBuilder& ToolbarBuilder);
	void FillEditorToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
	/** Pointer back to the code editor tool that owns us */
	TWeakPtr<class FScriptEditor> ScriptEditor;
};
