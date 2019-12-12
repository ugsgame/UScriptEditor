﻿// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"
#include "UObject/GCObject.h"
#include "Toolkits/IToolkitHost.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "SScriptDebugger.h"


class FScriptEditorToolbar;
class FDocumentTracker;
class FTabInfo;
class USourceProject;
class UCodeProjectItem;

class FScriptEditor : public FWorkflowCentricApplication, public FGCObject
{
public:
	FScriptEditor();

	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// End of IToolkit interface

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual bool OnRequestClose() override;
	// End of FAssetEditorToolkit

	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FSerializableObject interface

	static TSharedPtr<FScriptEditor> Get()
	{
		return ScriptEditor.Pin();
	}

public:
	/** Initialize the code editor */
	void InitScriptEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UCodeProjectItem* CodeProject, class UCodeProjectItem* ScriptProject);

	/** Try to open a new file for editing */
	void OpenFileForEditing(class UCodeProjectItem* Item);
	/** Try to open a new file for editing */
	void OpenFileAndGotoLine(class UCodeProjectItem* Item,int32 Line);
	/** Try to close a editing file */
	void CloseEditingFile(class UCodeProjectItem* Item);
	/** Try to close a all editing files */
	void CloseAllEditingFiles();

	/** Rescan all script files */
	void RescanProject();

	/** Get the current project being edited by this code editor */
	UCodeProjectItem* GetCodeProjectBeingEdited() const { return CodeProjectBeingEdited.Get(); }
	UCodeProjectItem* GetScriptProjectBeingEdited() const { return ScriptProjectBeingEdited.Get(); }

	TSharedRef<SWidget> CreateCodeEditorWidget(TSharedRef<FTabInfo> TabInfo, UCodeProjectItem* Item);

	void RegisterToolbarTab(const TSharedRef<class FTabManager>& TabManager);

	/** Access the toolbar builder for this editor */
	TSharedPtr<class FScriptEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }

	bool Save();
	bool SaveAll();
	bool Reload();
	bool ReloadAll();

private:
	void BindCommands();

	//File Actions
	void Save_Internal();
	void SaveAll_Internal();
	void Reload_Internal();
	void ReloadAll_Internal();

	bool CanSave() const;
	bool CanSaveAll() const;
	bool CanReload() const;
	bool CanReloadAll() const;
	//Debug Actions
	void DebugContinue();
	void DebugStepover();
	void DebugStepin();
	void DebugStepout();

	bool CanDebugContinue() const;
	bool CanDebugStepover() const;
	bool CanDebugStepin() const;
	bool CanDebugStepout() const;
	//
protected:
	//Overried Asset Editing
	virtual bool CanSaveAsset()const;
	virtual void SaveAsset_Execute();

	virtual void FindInContentBrowser_Execute();

	//
public:
	//Debug
	TMap<FString, TSet<int32>> EnableBreakPoint;
	TArray<FBreakPointNode_Ref> BreakPointForView;
	//
	bool HasBreakPoint(FString& FilePath, int32 CodeLine);
	void ToggleBreakPoint(FString& FilePath, int32 CodeLine);
	TSharedPtr<FScriptBreakPointNode> GetViewBreakPoint(FString& FilePath, int32 CodeLine);
	//
protected:
	TSharedPtr<FDocumentTracker> DocumentManager;

	/** The code project we are currently editing */
	TWeakObjectPtr<UCodeProjectItem> CodeProjectBeingEdited;
	TWeakObjectPtr<UCodeProjectItem> ScriptProjectBeingEdited;

	TSharedPtr<class FScriptEditorToolbar> ToolbarBuilder;

	static TWeakPtr<FScriptEditor> ScriptEditor;
};
