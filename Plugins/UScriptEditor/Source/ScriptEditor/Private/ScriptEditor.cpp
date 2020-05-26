// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditor.h"
#include "ScriptProjectItem.h"
#include "SourceProject.h"
#include "ScriptEditorStyle.h"
#include "ScriptEditorCommands.h"
#include "Editor/UnrealEd//Private/Toolkits/AssetEditorCommonCommands.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "SCodeEditor.h"
#include "SProjectTreeEditor.h"
#include "SScriptEditorLog.h"
#include "SScriptDebugger.h"
#include "VarWatcher.h"
#include "Widgets/Docking/SDockTab.h"
#include "ScriptEditorToolbar.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"
#include "ScriptEditorSetting.h"
#include "ScriptEditorModule.h"
#include "SGraphActionMenu.h"
#include "Kismet/Private/SBlueprintActionMenu.h"
#include "SScriptActionMenu.h"
#include "Framework/Application/SlateApplication.h"


#define LOCTEXT_NAMESPACE "ScriptEditor"

TWeakPtr<FScriptEditor> FScriptEditor::ScriptEditor;

const FName ScriptEditorAppName = FName(TEXT("ScriptEditorApp"));

namespace ScriptEditorModes
{
	// Mode identifiers
	static const FName StandardMode(TEXT("StandardMode"));
};

namespace ScriptEditorTabs
{
	// Tab identifiers
	static const FName ProjectViewID(TEXT("ProjectView"));
	static const FName CodeViewID(TEXT("Document"));
	static const FName LogViewID(TEXT("Log"));
	static const FName DebuggerViewID(TEXT("Debugger"));
	static const FName VarWathcerViewID(TEXT("VarWathcer"));
	static const FName APIBrowserViewID(TEXT("APIBrowser"));
};


struct FCodeTabSummoner : public FDocumentTabFactoryForObjects<UScriptProjectItem>
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SWidget>, FOnCreateScriptEditorWidget, TSharedRef<FTabInfo>, UScriptProjectItem*);

public:
	FCodeTabSummoner(TSharedPtr<class FScriptEditor> InCodeProjectEditorPtr, FOnCreateScriptEditorWidget CreateScriptEditorWidgetCallback)
		: FDocumentTabFactoryForObjects<UScriptProjectItem>(ScriptEditorTabs::CodeViewID, InCodeProjectEditorPtr)
		, ScriptEditorPtr(InCodeProjectEditorPtr)
		, OnCreateScriptEditorWidget(CreateScriptEditorWidgetCallback)
	{
	}

	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const override
	{
		TSharedRef<SCodeEditor> CodeEditor = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());
		if (UScriptProjectItem* Item = CodeEditor->GetCodeProjectItem())
		{
			if (SProjectTreeEditor::Get().IsValid())
			{
				//TODO:Switch to ScriptProject or SoucreProject
				//Expanded item
				SProjectTreeEditor::Get()->ExpanedEditingItem(Item);

				SProjectTreeEditor::Get()->ExpandedItems.Add(Item);
			}
		}
		//	InCodeProjectEditorPtr.Pin()->OnCodeEditorFocused(CodeEditor);

		Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FCodeTabSummoner::OnCloseTab));
	}

	virtual void OnTabRefreshed(TSharedPtr<SDockTab> Tab) const override
	{
		TSharedRef<SCodeEditor> GraphEditor = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());
		//	GraphEditor->NotifyItemChanged();
	}

	virtual void SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const override
	{
		TSharedRef<SCodeEditor> GraphEditor = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());

		//	UCodeProjectItem* Graph = FTabPayload_UObject::CastChecked<UCodeProjectItem>(Payload);
		//	BlueprintEditorPtr.Pin()->GetBlueprintObj()->LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
	}

	void OnCloseTab(TSharedRef<class SDockTab> Tab)
	{
		TSharedRef<SCodeEditor> CodeEditor = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());

		if (UScriptProjectItem* Item = CodeEditor->GetCodeProjectItem())
		{
			UScriptEdtiorSetting::Get()->EdittingFiles.Remove(Item->Path);

			if (SProjectTreeEditor::Get().IsValid())
			{
				SProjectTreeEditor::Get()->ExpandedItems.Remove(Item);
			}
		}
		CodeEditor->OnClose();
	}

protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UScriptProjectItem* DocumentID) const override
	{
		return FText::FromString(DocumentID->Name);
	}

	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UScriptProjectItem* DocumentID) const override
	{
		check(Info.TabInfo.IsValid());
		return OnCreateScriptEditorWidget.Execute(Info.TabInfo.ToSharedRef(), DocumentID);
	}

	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UScriptProjectItem* DocumentID) const override
	{
		if (DocumentID)
		{
			return FScriptEditorStyle::Get().GetBrush(DocumentID->GetBrush());
		}
		return FScriptEditorStyle::Get().GetBrush("ProjectEditor.Icon.File");
	}

	/*	virtual TSharedRef<FGenericTabHistory> CreateTabHistoryNode(TSharedPtr<FTabPayload> Payload) override
		{
			return MakeShareable(new FSourceTabHistory(SharedThis(this), Payload));
		}*/

protected:
	TWeakPtr<class FScriptEditor> ScriptEditorPtr;
	FOnCreateScriptEditorWidget OnCreateScriptEditorWidget;
};


struct FProjectViewSummoner : public FWorkflowTabFactory
{
public:
	FProjectViewSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(ScriptEditorTabs::ProjectViewID, InHostingApp)
	{
		TabLabel = NSLOCTEXT("ProjectTabLabel", "TabTitle", "Project");
		TabIcon = FSlateIcon(FScriptEditorStyle::Get().GetStyleSetName(), "ProjectEditor.Project");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("ProjectTabMenu_Description", "Project");
		ViewMenuTooltip = LOCTEXT("ProjectTabMenu_ToolTip", "Shows the project panel");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		TSharedPtr<FScriptEditor> CodeEditorPtr = StaticCastSharedPtr<FScriptEditor>(HostingApp.Pin());
		return SNew(SProjectTreeEditor, CodeEditorPtr->GetCodeProjectBeingEdited(), CodeEditorPtr->GetScriptProjectBeingEdited());
	}
};

struct FLogViewSummoner : public FWorkflowTabFactory
{
public:
	FLogViewSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(ScriptEditorTabs::LogViewID, InHostingApp)
	{
		TabLabel = NSLOCTEXT("ScriptConsole", "TabTitle", "Log");
		TabIcon = FSlateIcon(FScriptEditorStyle::Get().GetStyleSetName(), "ScriptEditor.Log");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("LogViewTabMenu_Description", "Log");
		ViewMenuTooltip = LOCTEXT("LogViewTabMenu_ToolTip", "Shows the script Logs");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		return SNew(SScriptEditorLog).Messages(FScriptEditorModule::GetInstance()->ScriptLogHistory->GetMessages());
	}
};

struct FDebuggerViewSummoner : public FWorkflowTabFactory
{
public:
	FDebuggerViewSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(ScriptEditorTabs::DebuggerViewID, InHostingApp)
	{
		TabLabel = NSLOCTEXT("ScriptDebugger", "TabTitle", "Debugger");
		TabIcon = FSlateIcon(FScriptEditorStyle::Get().GetStyleSetName(), "ScriptEditor.Debugger");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("DebuggerViewTabMenu_Description", "Debugger");
		ViewMenuTooltip = LOCTEXT("DebuggerViewTabMenu_ToolTip", "The Script Debugger");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		return SNew(SScriptDebugger);
	}

	virtual TSharedRef<SDockTab> SpawnTab(const FWorkflowTabSpawnInfo& Info) const override
	{
		TSharedRef<SDockTab> Tab = FWorkflowTabFactory::SpawnTab(Info);
		Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FDebuggerViewSummoner::OnTabClosed));

		return Tab;
	}

	void OnTabClosed(TSharedRef<class SDockTab> DebugTab)
	{
		SScriptDebugger::Get()->DebugTabClose(DebugTab);
	}
};

struct FVarWatcherViewSummoner : public FWorkflowTabFactory
{
public:
	FVarWatcherViewSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(ScriptEditorTabs::VarWathcerViewID, InHostingApp)
	{
		TabLabel = NSLOCTEXT("VarWatcher_Tile", "TabTitle", "VarWatcher");
		TabIcon = FSlateIcon(FScriptEditorStyle::Get().GetStyleSetName(), "ScriptEditor.VarWatcher");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("VarWatcherTabMenu_Description", "VarWatcher");
		ViewMenuTooltip = LOCTEXT("VarWatcherTabMenu_ToolTip", "Shows the script VarWatcher");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		return SNew(SVarWatcher);
	}

	virtual TSharedRef<SDockTab> SpawnTab(const FWorkflowTabSpawnInfo& Info) const override
	{
		TSharedRef<SDockTab> Tab = FWorkflowTabFactory::SpawnTab(Info);
		Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FVarWatcherViewSummoner::OnTabClosed));

		return Tab;
	}

	void OnTabClosed(TSharedRef<class SDockTab> DebugTab)
	{
		SVarWatcher::Get()->WatcherTabClose(DebugTab);
	}

};

struct FAPIBrowserViewSummoner : public FWorkflowTabFactory
{
public:
	FAPIBrowserViewSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(ScriptEditorTabs::APIBrowserViewID, InHostingApp)
	{
		TabLabel = NSLOCTEXT("APIBrowser_Tile", "TabTitle", "API Browser");
		TabIcon = FSlateIcon(FScriptEditorStyle::Get().GetStyleSetName(), "ScriptEditor.APIBroswer");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("APIBrowserTabMenu_Description", "API Browser");
		ViewMenuTooltip = LOCTEXT("APIBrowserTabMenu_ToolTip", "Shows the script API Browser");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		return SNew(SScriptActionMenu, FScriptEditor::Get());
		//return SNew(SBlueprintActionMenu);
	}
};

class FBasicScriptEditorMode : public FApplicationMode
{
public:
	FBasicScriptEditorMode(TSharedPtr<class FScriptEditor> InScriptEditor, FName InModeName);

	// FApplicationMode interface
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	// End of FApplicationMode interface

protected:
	TWeakPtr<FScriptEditor> MyScriptEditor;
	FWorkflowAllowedTabSet TabFactories;

};

FBasicScriptEditorMode::FBasicScriptEditorMode(TSharedPtr<class FScriptEditor> InScriptEditor, FName InModeName)
	: FApplicationMode(InModeName)
{
	MyScriptEditor = InScriptEditor;

	TabFactories.RegisterFactory(MakeShareable(new FProjectViewSummoner(InScriptEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FLogViewSummoner(InScriptEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FDebuggerViewSummoner(InScriptEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FVarWatcherViewSummoner(InScriptEditor)));
	TabFactories.RegisterFactory(MakeShareable(new FAPIBrowserViewSummoner(InScriptEditor)));

	TabLayout = FTabManager::NewLayout("Standalone_ScriptEditor_Layout_v1.1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(InScriptEditor->GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetSizeCoefficient(0.9f)
				->SetOrientation(Orient_Horizontal)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->SetHideTabWell(false)
					->AddTab(ScriptEditorTabs::VarWathcerViewID, ETabState::OpenedTab)
					->AddTab(ScriptEditorTabs::APIBrowserViewID, ETabState::OpenedTab)
					->AddTab(ScriptEditorTabs::ProjectViewID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetSizeCoefficient(0.8f)
					->SetOrientation(Orient_Vertical)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.7f)
						->SetHideTabWell(false)
						->AddTab(ScriptEditorTabs::CodeViewID, ETabState::ClosedTab)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.3f)
						->SetHideTabWell(false)
						->AddTab(ScriptEditorTabs::DebuggerViewID, ETabState::OpenedTab)
						->AddTab(ScriptEditorTabs::LogViewID, ETabState::OpenedTab)
					)
				)
			)
		);

	InScriptEditor->GetToolbarBuilder()->AddEditorToolbar(ToolbarExtender);
}

void FBasicScriptEditorMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FScriptEditor> Editor = MyScriptEditor.Pin();

	Editor->RegisterToolbarTab(InTabManager.ToSharedRef());

	Editor->PushTabFactories(TabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

FScriptEditor::FScriptEditor()
{
}

void FScriptEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	DocumentManager->SetTabManager(InTabManager);

	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

void FScriptEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FScriptEditor::InitScriptEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UScriptProjectItem* CodeProject, class UScriptProjectItem* ScriptProject)
{
#if ENGINE_MINOR_VERSION < 24
	FAssetEditorManager::Get().CloseOtherEditors(ScriptProject, this);
#else
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(ScriptProject, this);
#endif

	CodeProjectBeingEdited = CodeProject;
	ScriptProjectBeingEdited = ScriptProject;

	//
	for (FScriptBreakPointNode &Node : UScriptDebuggerSetting::Get()->RecentBreakPoint)
	{
		TSet<int32>& Set = EnableBreakPoint.FindOrAdd(Node.FilePath);
		Set.Add(Node.Line);
		FBreakPointNode_Ref NewNode = MakeShareable(new FScriptBreakPointNode(Node.Line, Node.FilePath));
		NewNode->HitCondition = Node.HitCondition;
		BreakPointForView.Add(NewNode);
	}
	//

	TSharedPtr<FScriptEditor> ThisPtr(SharedThis(this));
	if (!DocumentManager.IsValid())
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);
	}

	ScriptEditor = ThisPtr;

	TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FCodeTabSummoner(ThisPtr, FCodeTabSummoner::FOnCreateScriptEditorWidget::CreateSP(this, &FScriptEditor::CreateCodeEditorWidget)));
	DocumentManager->RegisterDocumentFactory(GraphEditorFactory);

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FScriptEditorToolbar(SharedThis(this)));
	}

	//FCodeProjectEditorCommands::Register();

	// Initialize the asset editor and spawn nothing (dummy layout)
	const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
	InitAssetEditor(Mode, InitToolkitHost, ScriptEditorAppName, DummyLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, ScriptProject);

	BindCommands();

	// Create the modes and activate one (which will populate with a real layout)
	AddApplicationMode(
		ScriptEditorModes::StandardMode,
		MakeShareable(new FBasicScriptEditorMode(ThisPtr, ScriptEditorModes::StandardMode)));

	SetCurrentMode(ScriptEditorModes::StandardMode);

	RegenerateMenusAndToolbars();
}

void FScriptEditor::BindCommands()
{

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().TestAction,
		FExecuteAction::CreateSP(this, &FScriptEditor::TestAction),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanTestAction)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().Save,
		FExecuteAction::CreateSP(this, &FScriptEditor::Save_Internal),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanSave)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().SaveAll,
		FExecuteAction::CreateSP(this, &FScriptEditor::SaveAll_Internal),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanSaveAll)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().Reload,
		FExecuteAction::CreateSP(this, &FScriptEditor::Reload_Internal),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanReload)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().ReloadAll,
		FExecuteAction::CreateSP(this, &FScriptEditor::ReloadAll_Internal),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanReloadAll)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().DebugContinue,
		FExecuteAction::CreateSP(this, &FScriptEditor::DebugContinue),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanDebugContinue)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().DebugStepover,
		FExecuteAction::CreateSP(this, &FScriptEditor::DebugStepover),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanDebugStepover)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().DebugStepin,
		FExecuteAction::CreateSP(this, &FScriptEditor::DebugStepin),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanDebugStepin)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().DebugStepout,
		FExecuteAction::CreateSP(this, &FScriptEditor::DebugStepout),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanDebugStepout)
	);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().DebugAbort,
		FExecuteAction::CreateSP(this, &FScriptEditor::DebugAbort),
		FCanExecuteAction::CreateSP(this, &FScriptEditor::CanDebugAbort)
	);
}

void FScriptEditor::OpenFileForEditing(UScriptProjectItem* Item)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(Item);
	DocumentManager->OpenDocument(Payload, FDocumentTracker::OpenNewDocument);

	UScriptEdtiorSetting::Get()->EdittingFiles.Add(Item->Path);
}

void FScriptEditor::OpenFileAndGotoLine(UScriptProjectItem* Item, int32 Line)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(Item);
	TSharedPtr<SDockTab> DocTab = DocumentManager->OpenDocument(Payload, FDocumentTracker::OpenNewDocument);

	if (DocTab.IsValid())
	{
		TSharedRef<SCodeEditor> CodeEditor = StaticCastSharedRef<SCodeEditor>(DocTab->GetContent());
		CodeEditor->GotoLineAndColumn(Line - 1, 0);
	}

	UScriptEdtiorSetting::Get()->EdittingFiles.Add(Item->Path);
}

void FScriptEditor::CloseEditingFile(UScriptProjectItem* Item)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(Item);
	DocumentManager->CloseTab(Payload);

	//UScriptEdtiorSetting::Get()->EdittingFiles.Remove(Item->Path);
}

void FScriptEditor::CloseAllEditingFiles()
{
	TArray<TSharedPtr<SDockTab>> Tabs = DocumentManager->GetAllDocumentTabs();
	for (TSharedPtr<SDockTab> Tab : Tabs)
	{
		if (Tab.IsValid())
		{
			Tab->RequestCloseTab();
		}
	}
}

void FScriptEditor::RescanProject()
{
	TSharedPtr<SProjectTreeEditor> ProjectTreeEditor = SProjectTreeEditor::Get();
	if (ProjectTreeEditor.IsValid())
	{
		ProjectTreeEditor->RescanScripts();
		ProjectTreeEditor->RescanSources();
		ProjectTreeEditor->RequestRefresh();
	}
}

FName FScriptEditor::GetToolkitFName() const
{
	return FName("UScriptEditor");
}

FText FScriptEditor::GetBaseToolkitName() const
{
	return LOCTEXT("UScriptEditorAppLabel", "UScriptEditor");
}

FText FScriptEditor::GetToolkitName() const
{
	return LOCTEXT("UScriptEditorAppToolkitName", "UScriptEditor");
}

FText FScriptEditor::GetToolkitToolTipText() const
{
	return LOCTEXT("UScriptEditorAppLabel", "UScriptEditor");
}

FString FScriptEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("UScriptEditor");
}

bool FScriptEditor::OnRequestClose()
{
	//Flash BreakPoints
	UScriptDebuggerSetting::Get()->RecentBreakPoint.Reset();

	for (FBreakPointNode_Ref &Node : BreakPointForView)
	{
		UScriptDebuggerSetting::Get()->RecentBreakPoint.Add(*Node);
	}
	//
	FScriptEditorModule::GetInstance()->SaveConfig();

	//remove unlua bind in UScriptDebuggerSetting
	UScriptDebuggerSetting::Get()->SetTabIsOpen(false);
	
	UVarWatcherSetting::Get()->SetTapIsOpen(false);

	return	FWorkflowCentricApplication::OnRequestClose();
}

FLinearColor FScriptEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FScriptEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	UScriptProjectItem* CodeProject = CodeProjectBeingEdited.Get();
	Collector.AddReferencedObject(CodeProject);

	UScriptProjectItem* ScriptProject = ScriptProjectBeingEdited.Get();
	Collector.AddReferencedObject(ScriptProject);
}

TSharedRef<SWidget> FScriptEditor::CreateCodeEditorWidget(TSharedRef<FTabInfo> TabInfo, UScriptProjectItem* Item)
{
	return SNew(SCodeEditor, Item);
}

void FScriptEditor::Save_Internal()
{
	Save();
}

bool FScriptEditor::Save()
{
	if (DocumentManager.IsValid() && DocumentManager->GetActiveTab().IsValid())
	{
		TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(DocumentManager->GetActiveTab()->GetContent());
		return CodeEditorRef->Save();
	}

	return false;
}

bool FScriptEditor::CanSave() const
{
	if (DocumentManager.IsValid() && DocumentManager->GetActiveTab().IsValid())
	{
		TSharedRef<SWidget> Content = DocumentManager->GetActiveTab()->GetContent();
		TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(Content);
		return CodeEditorRef->CanSave();
	}

	return false;
}

void FScriptEditor::SaveAll_Internal()
{
	SaveAll();
}

void FScriptEditor::Reload_Internal()
{
	if (!Reload())
	{
		//TODO:Show a error dialog
	}
}

void FScriptEditor::ReloadAll_Internal()
{
	ReloadAll();
}

void FScriptEditor::TestAction()
{
	//US_Log("Test Action");
}

bool FScriptEditor::CanTestAction() const
{
	return true;
}

bool FScriptEditor::SaveAll()
{
	bool bResult = true;

	if (DocumentManager.IsValid())
	{
		TArray<TSharedPtr<SDockTab>> AllTabs = DocumentManager->GetAllDocumentTabs();
		for (auto& Tab : AllTabs)
		{
			if (Tab.IsValid())
			{
				TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());
				if (!CodeEditorRef->Save())
				{
					bResult = false;
				}
			}
		}
	}

	return bResult;
}

bool FScriptEditor::Reload()
{
	if (DocumentManager.IsValid() && DocumentManager->GetActiveTab().IsValid())
	{
		TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(DocumentManager->GetActiveTab()->GetContent());
		return CodeEditorRef->Reload();
	}
	return true;
}

bool FScriptEditor::ReloadAll()
{
	RescanProject();
	return true;
}

bool FScriptEditor::CanSaveAll() const
{
	return true;
}

bool FScriptEditor::CanReload() const
{
	if (SScriptDebugger::Get())
	{
		return !SScriptDebugger::Get()->IsEnterDebugMode;
	}
	return true;
}

bool FScriptEditor::CanReloadAll() const
{
	if (SScriptDebugger::Get())
	{
		return !SScriptDebugger::Get()->IsEnterDebugMode;
	}
	return true;
}

void FScriptEditor::DebugContinue()
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->DebugContinue();
	}
}

void FScriptEditor::DebugStepover()
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->DebugStepover();
	}
}

void FScriptEditor::DebugStepin()
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->DebugStepin();
	}
}

void FScriptEditor::DebugStepout()
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->DebugStepout();
	}
}

void FScriptEditor::DebugAbort()
{
	//TODO:
	//Always leave debugging mode
	FSlateApplication::Get().LeaveDebuggingMode();
	//DebugStepout if enter debug mode
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->DebugContinue();
	}
}

bool FScriptEditor::CanDebugContinue()const
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->IsEnterDebugMode;
	}
	return false;
}

bool FScriptEditor::CanDebugStepover() const
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->IsEnterDebugMode;
	}
	return false;
}

bool FScriptEditor::CanDebugStepin() const
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->IsEnterDebugMode;
	}
	return false;
}

bool FScriptEditor::CanDebugStepout() const
{
	if (SScriptDebugger::Get())
	{
		return SScriptDebugger::Get()->IsEnterDebugMode;
	}
	return false;
}

bool FScriptEditor::CanDebugAbort() const
{
	return true;
}

bool FScriptEditor::CanSaveAsset()const
{
	return CanSave();
}

void FScriptEditor::SaveAsset_Execute()
{
	Save();
}

void FScriptEditor::FindInContentBrowser_Execute()
{
	if (DocumentManager.IsValid() && DocumentManager->GetActiveTab().IsValid())
	{
		TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(DocumentManager->GetActiveTab()->GetContent());
		CodeEditorRef->Browser();
	}
}

//////////////////////////////////////////////////////////////////////////
bool FScriptEditor::HasBreakPoint(FString& FilePath, int32 CodeLine)
{
	if (TSet<int32>* p = EnableBreakPoint.Find(FilePath))
	{
		if (p->Contains(CodeLine))
			return true;
	}
	return false;
}

void FScriptEditor::ToggleBreakPoint(FString& FilePath, int32 CodeLine)
{
	TSet<int32>& Set = EnableBreakPoint.FindOrAdd(FilePath);
	if (Set.Contains(CodeLine))
	{
		Set.Remove(CodeLine);

		for (int32 i = 0; i < BreakPointForView.Num(); i++)
		{
			FBreakPointNode_Ref&Node = BreakPointForView[i];
			if (Node->FilePath == FilePath && Node->Line == CodeLine)
			{
				BreakPointForView.RemoveAt(i);
				break;
			}
		}
	}
	else
	{
		Set.Add(CodeLine);
		FBreakPointNode_Ref NewNode = MakeShareable(new FScriptBreakPointNode(CodeLine, FilePath));
		BreakPointForView.Add(NewNode);
	}

	if (SScriptDebugger::Get())
	{
		SScriptDebugger::Get()->UpdateBreakPoints();
	}
}

TSharedPtr<FScriptBreakPointNode> FScriptEditor::GetViewBreakPoint(FString& FilePath, int32 CodeLine)
{
	for (int32 i = 0; i < BreakPointForView.Num(); i++)
	{
		FBreakPointNode_Ref& Node = BreakPointForView[i];
		if (Node->FilePath == FilePath && Node->Line == CodeLine)
		{
			return Node;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
