// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditor.h"
#include "CodeProjectItem.h"
#include "SourceProject.h"
#include "ScriptEditorStyle.h"
#include "ScriptEditorCommands.h"
#include "Editor/UnrealEd//Private/Toolkits/AssetEditorCommonCommands.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "SCodeEditor.h"
#include "SProjectTreeEditor.h"
#include "SScriptEditorLog.h"
#include "Widgets/Docking/SDockTab.h"
#include "ScriptEditorToolbar.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"


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
};


struct FCodeTabSummoner : public FDocumentTabFactoryForObjects<UCodeProjectItem>
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(TSharedRef<SWidget>, FOnCreateScriptEditorWidget, TSharedRef<FTabInfo>, UCodeProjectItem*);

public:
	FCodeTabSummoner(TSharedPtr<class FScriptEditor> InCodeProjectEditorPtr, FOnCreateScriptEditorWidget CreateScriptEditorWidgetCallback)
		: FDocumentTabFactoryForObjects<UCodeProjectItem>(ScriptEditorTabs::CodeViewID, InCodeProjectEditorPtr)
		, ScriptEditorPtr(InCodeProjectEditorPtr)
		, OnCreateScriptEditorWidget(CreateScriptEditorWidgetCallback)
	{
	}

	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const override
	{
		TSharedRef<SCodeEditor> CodeEditor = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());
	//	InCodeProjectEditorPtr.Pin()->OnCodeEditorFocused(CodeEditor);
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

protected:
	virtual TAttribute<FText> ConstructTabNameForObject(UCodeProjectItem* DocumentID) const override
	{
		return FText::FromString(DocumentID->Name);
	}

	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UCodeProjectItem* DocumentID) const override
	{
		check(Info.TabInfo.IsValid());
		return OnCreateScriptEditorWidget.Execute(Info.TabInfo.ToSharedRef(), DocumentID);
	}

	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UCodeProjectItem* DocumentID) const override
	{
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
		TabLabel = LOCTEXT("ProjectTabLabel", "Project");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("ProjectTabMenu_Description", "Project");
		ViewMenuTooltip = LOCTEXT("ProjectTabMenu_ToolTip", "Shows the project panel");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
		TSharedPtr<FScriptEditor> CodeEditorPtr = StaticCastSharedPtr<FScriptEditor>(HostingApp.Pin());
		return SNew(SProjectTreeEditor, CodeEditorPtr->GetCodeProjectBeingEdited(),CodeEditorPtr->GetScriptProjectBeingEdited());
	}
};

struct FLogViewSummoner : public FWorkflowTabFactory
{
public:
	FLogViewSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(ScriptEditorTabs::LogViewID, InHostingApp)
	{
		TabLabel = NSLOCTEXT("ScriptConsole", "TabTitle", "Script Log");
		//TabIcon = FEditorStyle::GetBrush("Log.TabIcon");

		bIsSingleton = true;

		ViewMenuDescription = LOCTEXT("LogViewTabMenu_Description", "Log");
		ViewMenuTooltip = LOCTEXT("LogViewTabMenu_ToolTip", "Shows the script Logs");
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override
	{
// 		return SNew(SDockTab)
// 			.Icon(FEditorStyle::GetBrush("Log.TabIcon"))
// 			.TabRole(ETabRole::NomadTab)
// 			.Label(NSLOCTEXT("ScriptConsole", "TabTitle", "Script Console"))
// 			[
// 				SNew(SScriptEditorLog).Messages(FScriptEditorModule::GetInstance()->PythonLogHistory->GetMessages())
// 			
		return SNew(SScriptEditorLog).Messages(FScriptEditorModule::GetInstance()->ScriptLogHistory->GetMessages());
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
					->SetHideTabWell(true)
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

void FScriptEditor::InitScriptEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UCodeProjectItem* CodeProject, class UCodeProjectItem* ScriptProject)
{
	//FAssetEditorManager::Get().CloseOtherEditors(CodeProject, this);
	FAssetEditorManager::Get().CloseOtherEditors(ScriptProject, this);
	CodeProjectBeingEdited = CodeProject;
	ScriptProjectBeingEdited = ScriptProject;

	TSharedPtr<FScriptEditor> ThisPtr(SharedThis(this));
	if(!DocumentManager.IsValid())
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
	ToolkitCommands->MapAction(FScriptEditorCommands::Get().Save,
			FExecuteAction::CreateSP(this, &FScriptEditor::Save_Internal),
			FCanExecuteAction::CreateSP(this, &FScriptEditor::CanSave)
			);

	ToolkitCommands->MapAction(FScriptEditorCommands::Get().SaveAll,
			FExecuteAction::CreateSP(this, &FScriptEditor::SaveAll_Internal),
			FCanExecuteAction::CreateSP(this, &FScriptEditor::CanSaveAll)
			);
}

void FScriptEditor::OpenFileForEditing(UCodeProjectItem* Item)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(Item);
	DocumentManager->OpenDocument(Payload, FDocumentTracker::OpenNewDocument);
}

void FScriptEditor::CloseEditingFile(UCodeProjectItem* Item)
{
	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(Item);
	DocumentManager->CloseTab(Payload);
}

void FScriptEditor::CloseAllEditingFiles()
{
	TArray<TSharedPtr<SDockTab>> Tabs = DocumentManager->GetAllDocumentTabs();
	for (TSharedPtr<SDockTab> Tab:Tabs)
	{
		if (Tab.IsValid())
		{
			Tab->RequestCloseTab();
		}
	}
}

void FScriptEditor::RescanScriptProject()
{
	this->CloseAllEditingFiles();
	TSharedPtr<SProjectTreeEditor> ProjectTreeEditor = SProjectTreeEditor::Get();
	if (ProjectTreeEditor.IsValid())
	{
		ProjectTreeEditor->RescanScripts();
		ProjectTreeEditor->RequestRefresh();
	}
	//TODO:
	//Open all old edited files
	//
	//
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

FLinearColor FScriptEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

void FScriptEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	UCodeProjectItem* CodeProject = CodeProjectBeingEdited.Get();
	Collector.AddReferencedObject(CodeProject);

	UCodeProjectItem* ScriptProject = ScriptProjectBeingEdited.Get();
	Collector.AddReferencedObject(ScriptProject);
}

TSharedRef<SWidget> FScriptEditor::CreateCodeEditorWidget(TSharedRef<FTabInfo> TabInfo, UCodeProjectItem* Item)
{
	return SNew(SCodeEditor, Item);
}

void FScriptEditor::Save_Internal()
{
	Save();
}

bool FScriptEditor::Save()
{
	if(DocumentManager.IsValid() && DocumentManager->GetActiveTab().IsValid())
	{
		TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(DocumentManager->GetActiveTab()->GetContent());
		return CodeEditorRef->Save();
	}

	return false;
}

bool FScriptEditor::CanSave() const
{
	if(DocumentManager.IsValid() && DocumentManager->GetActiveTab().IsValid())
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

bool FScriptEditor::SaveAll()
{
	bool bResult = true;

	if(DocumentManager.IsValid())
	{
		TArray<TSharedPtr<SDockTab>> AllTabs = DocumentManager->GetAllDocumentTabs();
		for(auto& Tab : AllTabs)
		{
			if(Tab.IsValid())
			{
				TSharedRef<SCodeEditor> CodeEditorRef = StaticCastSharedRef<SCodeEditor>(Tab->GetContent());
				if(!CodeEditorRef->Save())
				{
					bResult = false;
				}
			}
		}
	}

	return bResult;
}

bool FScriptEditor::CanSaveAll() const
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

#undef LOCTEXT_NAMESPACE
