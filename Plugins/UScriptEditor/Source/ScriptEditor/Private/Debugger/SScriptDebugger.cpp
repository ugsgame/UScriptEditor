// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SScriptDebugger.h"
//#include "LuaDebuggerStyle.h"
//#include "LuaDebuggerCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "FileManager.h"
#include "FileHelper.h"
#include "UScriptDebuggerSetting.h"
#include "SlateApplication.h"
#include "SSearchBox.h"
#include "CoreDelegates.h"
#include "SEditableTextBox.h"

static const FName LuaDebuggerTabName("LuaDebugger");
static const FName DefaultForegroundName("DefaultForeground");

#define LOCTEXT_NAMESPACE "SScriptDebugger"


SScriptDebugger* SScriptDebugger::Ptr;

void SScriptDebugger::Construct(const FArguments& InArgs)
{
	StartupModule();

	OnSpawnPluginTab(FSpawnTabArgs(TSharedPtr<SWindow>(), FTabId()));
}

void SScriptDebugger::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	Ptr = this;
	IsDebugRun = true;
	IsDebugRemote = false;
	IsEnterDebugMode = false;
	IntervalToCheckFileChange = 0;
	ptr_HandleKeyDown = MakeShareable(new FHandleKeyDown());
	StackListState = EStackListState::CallStack;
	LastTimeFileOffset = UScriptDebuggerSetting::Get(IsDebugRemote)->LastTimeFileOffset;
	RecentFilePath = UScriptDebuggerSetting::Get(IsDebugRemote)->RecentFilePath;
	for (FScriptBreakPointNode &Node : UScriptDebuggerSetting::Get(IsDebugRemote)->RecentBreakPoint)
	{
		TSet<int32>& Set = EnableBreakPoint.FindOrAdd(Node.FilePath);
		Set.Add(Node.Line);
		FBreakPointNode_Ref NewNode = MakeShareable(new FScriptBreakPointNode(Node.Line, Node.FilePath));
		NewNode->HitCondition = Node.HitCondition;
		BreakPointForView.Add(NewNode);
	}

	FCoreDelegates::OnPreExit.AddRaw(this, &SScriptDebugger::BeforeExit);
	/*
	FLuaDebuggerStyle::Initialize();
	FLuaDebuggerStyle::ReloadTextures();

	FLuaDebuggerCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	/*
	PluginCommands->MapAction(
		FLuaDebuggerCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &SDebugger::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &SDebugger::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	// 	{
	// 		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	// 		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &SDebugger::AddToolbarExtension));
	// 		
	// 		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	// 	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(LuaDebuggerTabName, FOnSpawnTab::CreateRaw(this, &SDebugger::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FLuaDebuggerTabTitle", "LuaDebugger"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
	*/
}

void SScriptDebugger::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	//FLuaDebuggerStyle::Shutdown();

	//FLuaDebuggerCommands::Unregister();

	//FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(LuaDebuggerTabName);
}

void SScriptDebugger::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	UScriptDebuggerSetting::Get(false)->SetTabIsOpen(true);
	UScriptDebuggerSetting::Get(true)->SetTabIsOpen(true);
	SyncState();
	FSlateApplication::Get().RegisterInputPreProcessor(ptr_HandleKeyDown);

// 	TSharedRef<SDockTab> Tab = SNew(SDockTab)
// 		.TabRole(ETabRole::NomadTab)
// 		.OnTabClosed_Raw(this, &SDebugger::DebugTabClose)
	TSharedPtr<SOverlay>OverlayWidget; this->ChildSlot
		[
			SAssignNew(OverlayWidget, SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
						.IsChecked_Lambda([&]() {return IsDebugRun ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Raw(this, &SScriptDebugger::ToggleStartDebug)
						[
							SNew(SBox)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.Padding(FMargin(4.0, 2.0))
							[
								SNew(STextBlock)
								.Text(FText::FromString("Start"))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SCheckBox)
						.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
						.IsChecked_Lambda([&]() {return IsDebugRemote ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Raw(this, &SScriptDebugger::ToggleRemoteDebug)
						[
							SNew(SBox)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							.Padding(FMargin(4.0, 2.0))
							[
								SNew(STextBlock)
								.Text(FText::FromString("Remote"))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &SScriptDebugger::DebugContinue)
						[
							SNew(STextBlock)
							.Text(FText::FromString("Continue F5"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &SScriptDebugger::DebugStepover)
						[
							SNew(STextBlock)
							.Text(FText::FromString("StepOver F10"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &SScriptDebugger::DebugStepin)
						[
							SNew(STextBlock)
							.Text(FText::FromString("StepIn F11"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.OnClicked_Raw(this, &SScriptDebugger::DebugStepout)
						[
							SNew(STextBlock)
							.Text(FText::FromString("StepOut shift+F11"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SButton)
							.VAlign(EVerticalAlignment::VAlign_Center)
							.ButtonStyle(FEditorStyle::Get(), "FlatButton")
							.ForegroundColor(FEditorStyle::GetSlateColor(DefaultForegroundName))
		// 					.ToolTipText(this, &SContentBrowser::GetHistoryBackTooltip)
							.ContentPadding(FMargin(1, 0))
		// 					.OnClicked(this, &SContentBrowser::BackClicked)
		// 					.IsEnabled(this, &SContentBrowser::IsBackEnabled)
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
								.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
								.Text(FText::FromString(FString(TEXT("\xf060"))) /*fa-arrow-left*/)
							]
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.FillHeight(1.0f)
						[
							SNew(SButton)
							.VAlign(EVerticalAlignment::VAlign_Center)
							.ButtonStyle(FEditorStyle::Get(), "FlatButton")
							.ForegroundColor(FEditorStyle::GetSlateColor(DefaultForegroundName))
		// 					.ToolTipText(this, &SContentBrowser::GetHistoryBackTooltip)
							.ContentPadding(FMargin(1, 0))
		// 					.OnClicked(this, &SContentBrowser::BackClicked)
		// 					.IsEnabled(this, &SContentBrowser::IsBackEnabled)
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
								.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
								.Text(FText::FromString(FString(TEXT("\xf061"))) /*fa-arrow-left*/)
							]
						]
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SSearchBox)
						.HintText(FText::FromString(TEXT("Line")))
						.OnTextChanged_Raw(this, &SScriptDebugger::OnSearchLineChange)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text_Lambda([&]()->FText {return FText::FromString(NowLuaCodeFilePath.Mid(GetLuaSourceDir().Len() + 1)); })
						]
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SSplitter)
					.Orientation(Orient_Vertical)
					+ SSplitter::Slot()
					.Value(0.7f)
					[
						SNew(SSplitter)
						.Orientation(Orient_Horizontal)
						+ SSplitter::Slot()
						.Value(0.2f)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SSearchBox)
									.HintText(FText::FromString(TEXT("Search...")))
									.OnTextChanged_Raw(this, &SScriptDebugger::OnFileFilterTextChanged)
	// 								.OnTextCommitted(this, &SSceneOutliner::OnFilterTextCommitted)
								]
								+ SVerticalBox::Slot()
								[
									SAssignNew(LuaFileTreePtr, SLuaFileTree)
									.ItemHeight(24.0f)
									.TreeItemsSource(&AfterFilterLuaFiles)
									.OnGenerateRow_Raw(this, &SScriptDebugger::HandleFileTreeGenerateRow)
									.OnGetChildren_Raw(this, &SScriptDebugger::HandleFileTreeGetChildren)
									.OnSelectionChanged_Raw(this, &SScriptDebugger::HandleFileTreeSelectionChanged)
									.OnExpansionChanged_Raw(this, &SScriptDebugger::HandleFileNodeExpansion)
								]
							]
						]
						+ SSplitter::Slot()
						.Value(0.8f)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							.Padding(FMargin(2.0f, 2.0f, 2.0f, 2.0f))
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("MessageLog.ListBorder"))
								[
									SAssignNew(LuaCodeListPtr, SLuaCodeList)
									.ItemHeight(24.0f)
									.ListItemsSource(&NowLuaCodes)
									.OnGenerateRow_Raw(this, &SScriptDebugger::HandleCodeListGenerateRow)
									.OnSelectionChanged_Raw(this, &SScriptDebugger::HandleCodeListSelectionChanged)
								]
							]
						]
					]
					+ SSplitter::Slot()
					.Value(0.3f)
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[
							SNew(SSplitter)
							.Orientation(Orient_Horizontal)
							+ SSplitter::Slot()
							.Value(0.618f)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("MessageLog.ListBorder"))
								[
									SAssignNew(DebuggerVarTree, SDebuggerVarTree)
									.ItemHeight(24.0f)
									.TreeItemsSource(&NowVars)
									.OnGenerateRow_Raw(this, &SScriptDebugger::HandleVarsTreeGenerateRow)
									.OnGetChildren_Raw(this, &SScriptDebugger::HandleVarsTreeGetChildren)
									.OnSelectionChanged_Raw(this, &SScriptDebugger::HandleVarsTreeSelectionChanged)
									.HeaderRow
									(
										SNew(SHeaderRow)
										+ SHeaderRow::Column(SDebuggerVarTreeWidgetItem::Col_Name)
										.DefaultLabel(LOCTEXT("VarName1", "Key"))
										.FillWidth(20.0f)

										+ SHeaderRow::Column(SDebuggerVarTreeWidgetItem::Col_Value)
										.DefaultLabel(LOCTEXT("VarValue1", "Value"))
										.FillWidth(20.0f)
									)
								]
							]
							+ SSplitter::Slot()
							.Value(1.0f - 0.618f)
							[
								SNew(SBorder)
								.BorderImage(FEditorStyle::GetBrush("MessageLog.ListBorder"))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									[
										SAssignNew(LuaStackListPtr, SLuaStackList)
										.Visibility_Lambda([&]()->EVisibility {return (StackListState == EStackListState::CallStack) ? EVisibility::Visible : EVisibility::Collapsed; })
										.ItemHeight(24.0f)
										.ListItemsSource(&NowLuaStack)
										.SelectionMode(ESelectionMode::Single)
										.OnGenerateRow_Raw(this, &SScriptDebugger::HandleStackListGenerateRow)
										.OnSelectionChanged_Raw(this, &SScriptDebugger::HandleStackListSelectionChanged)
									]
									+ SVerticalBox::Slot()
									[
										SAssignNew(BreakPointListPtr, SBreakPointList)
										.Visibility_Lambda([&]()->EVisibility {return (StackListState == EStackListState::BreakPoints) ? EVisibility::Visible : EVisibility::Collapsed; })
										.ItemHeight(24.0f)
										.ListItemsSource(&BreakPointForView)
										.SelectionMode(ESelectionMode::Single)
										.OnGenerateRow_Raw(this, &SScriptDebugger::HandleBreakPointListGenerateRow)
										.OnSelectionChanged_Raw(this, &SScriptDebugger::HandleBreakPointListSelectionChanged)
									]
									+ SVerticalBox::Slot()
									.VAlign(VAlign_Bottom)
									.AutoHeight()
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot()
										.HAlign(HAlign_Left)
										.AutoWidth()
										[
											SNew(SCheckBox)
											.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
											.IsChecked_Lambda([&]() {return (StackListState == EStackListState::CallStack) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
											.OnCheckStateChanged_Raw(this, &SScriptDebugger::ToggleStackListState, EStackListState::CallStack)
											[
												SNew(SBox)
												.VAlign(VAlign_Center)
												.HAlign(HAlign_Center)
												.Padding(FMargin(4.0, 2.0))
												[
													SNew(STextBlock)
													.Text(FText::FromString("CallStack"))
												]
											]
										]
										+ SHorizontalBox::Slot()
										.HAlign(HAlign_Left)
										.AutoWidth()
										[
											SNew(SCheckBox)
											.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
											.IsChecked_Lambda([&]() {return (StackListState == EStackListState::BreakPoints) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
											.OnCheckStateChanged_Raw(this, &SScriptDebugger::ToggleStackListState, EStackListState::BreakPoints)
											[
												SNew(SBox)
												.VAlign(VAlign_Center)
												.HAlign(HAlign_Center)
												.Padding(FMargin(4.0, 2.0))
												[
													SNew(STextBlock)
													.Text(FText::FromString("Breakpoints"))
												]
											]
										]
									]
								]
							]
						]
					]
				]
			]
		];
	ShowCode(RecentFilePath);
	//return Tab;
}

void SScriptDebugger::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(LuaDebuggerTabName);
}


bool SScriptDebugger::HasBreakPoint(FString& FilePath, int32 CodeLine)
{
	if (TSet<int32>* p = EnableBreakPoint.Find(FilePath))
	{
		if (p->Contains(CodeLine))
			return true;
	}
	return false;
}

void SScriptDebugger::ToggleBreakPoint(FString& FilePath, int32 CodeLine)
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
	BreakPointChange();
}


TSharedPtr<FScriptBreakPointNode> SScriptDebugger::GetViewBreakPoint(FString& FilePath, int32 CodeLine)
{
	for (int32 i = 0; i < BreakPointForView.Num(); i++)
	{
		FBreakPointNode_Ref&Node = BreakPointForView[i];
		if (Node->FilePath == FilePath && Node->Line == CodeLine)
		{
			return Node;
		}
	}
	return nullptr;
}

FText SScriptDebugger::GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine)
{
	TSharedPtr<FScriptBreakPointNode> BreakPointNode = GetViewBreakPoint(FilePath, CodeLine);
	FText BreakCondition;
	if (BreakPointNode.IsValid())
		BreakCondition = BreakPointNode->HitCondition;
	return BreakCondition;
}

void SScriptDebugger::ToggleStartDebug(const ECheckBoxState NewState)
{
	IsDebugRun = (NewState == ECheckBoxState::Checked) ? true : false;
	DebugStateChange();
}

void SScriptDebugger::ToggleRemoteDebug(const ECheckBoxState NewState)
{
	IsDebugRemote = (NewState == ECheckBoxState::Checked) ? true : false;
	RemoteStateChange();
}

void SScriptDebugger::ToggleStackListState(const ECheckBoxState NewState, EStackListState State)
{
	if (NewState == ECheckBoxState::Checked)
	{
		StackListState = State;
	}
}

void SScriptDebugger::BreakPointChange()
{
	if (BreakPointListPtr.IsValid())
		BreakPointListPtr.Pin()->RequestListRefresh();
	RefreshCodeList();
	UScriptDebuggerSetting::Get(true)->UpdateBreakPoint(EnableBreakPoint);
	UScriptDebuggerSetting::Get(false)->UpdateBreakPoint(EnableBreakPoint);
}


void SScriptDebugger::DebugStateChange()
{
	DebugContinue();
	UScriptDebuggerSetting::Get(true)->ToggleDebugStart(IsDebugRun && IsDebugRemote);
	UScriptDebuggerSetting::Get(false)->ToggleDebugStart(IsDebugRun && !IsDebugRemote);
}

void SScriptDebugger::RemoteStateChange()
{
	DebugStateChange();
}

void SScriptDebugger::SyncState()
{
	// 	RemoteStateChange();
	DebugStateChange();
	BreakPointChange();
	RefreshLuaFileData();
}

void SScriptDebugger::RefreshCodeList()
{
	if (LuaCodeListPtr.IsValid())
		LuaCodeListPtr.Pin()->RequestListRefresh();
}


void SScriptDebugger::RefreshStackList()
{
	if (LuaStackListPtr.IsValid())
	{
		LuaStackListPtr.Pin()->RequestListRefresh();
	}
}

FString SScriptDebugger::GetLuaSourceDir()
{
	//return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / "Plugins" / "UnrealLua" / TEXT("LuaSource"));
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / "Scripts");
}

FString SScriptDebugger::LuaPathToFilePath(FString LuaFilePath)
{
	if (LuaFilePath.Find("LuaSource"))
	{
		LuaFilePath.Replace(TEXT("\\"), TEXT("/"));
	}
	else
	{
		if (LuaFilePath.Find("."))
		{
			LuaFilePath.Replace(TEXT("."), TEXT("/"));
			LuaFilePath += ".lua";
			LuaFilePath = GetLuaSourceDir() / LuaFilePath;
		}
	}
	return LuaFilePath;
}

void SScriptDebugger::EnterDebug(const FString& LuaFilePath, int32 Line)
{
	ShowCode(LuaFilePath, Line);
	// todo
	if (NowLuaStack.Num() > 0)
		ShowStackVars(NowLuaStack[0]->StackIndex);
	IsEnterDebugMode = true;
	FSlateApplication::Get().EnterDebuggingMode();
}


void SScriptDebugger::SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos)
{
	NowLuaStack.Reset();
	for (int i = 0; i < Content.Num(); i++)
	{
		FStackListNode_Ref NewNode = MakeShareable(new FStackListNode(Content[i], Lines[i], FilePaths[i], StackIndex[i], FuncInfos[i]));
		NowLuaStack.Add(NewNode);
	}
	RefreshStackList();
}

void SScriptDebugger::ShowCode(const FString& FilePath, int32 Line /*= 0*/)
{
	if (LuaCodeListPtr.IsValid())
	{
		float NowOffset = LuaCodeListPtr.Pin()->GetScrollOffset();
		LastTimeFileOffset.Add(NowLuaCodeFilePath, NowOffset);
	}
	RecentFilePath = FilePath;
	NowLuaCodeFilePath = FilePath;
	ModifyTimeOfNowFile = IFileManager::Get().GetTimeStamp(*NowLuaCodeFilePath);
	FString RawCode;
	FFileHelper::LoadFileToString(RawCode, *FilePath);
	TArray<FString> CodeArr;
	RawCode.ParseIntoArrayLines(CodeArr, false);
	NowLuaCodes.Reset();
	for (int32 i = 0; i < CodeArr.Num(); i++)
	{
		FString& Code = CodeArr[i];
		FCodeListNode_Ref NewNode = MakeShareable(new FCodeListNode(Code, i + 1, FilePath));
		NowLuaCodes.Add(NewNode);
	}
	if (LuaCodeListPtr.IsValid())
	{
		if (float* Offset = LastTimeFileOffset.Find(NowLuaCodeFilePath))
		{
			LuaCodeListPtr.Pin()->SetScrollOffset(*Offset);
		}
		else
		{
			LuaCodeListPtr.Pin()->SetScrollOffset(0);
		}
	}
	RefreshCodeList();

	if (Line > 0)
	{
		if (LuaCodeListPtr.IsValid() && NowLuaCodes.IsValidIndex(Line - 1))
		{
			LuaCodeListPtr.Pin()->ClearSelection();
			LuaCodeListPtr.Pin()->SetItemSelection(NowLuaCodes[Line - 1], true);
			float NowOffset = LuaCodeListPtr.Pin()->GetScrollOffset();
			// todo
			if (NowOffset > Line || Line > NowOffset + 15)
				LuaCodeListPtr.Pin()->SetScrollOffset(Line - 5);
		}

	}
}


void SScriptDebugger::ShowStackVars(int32 StackIndex)
{
	if (StackIndex == -1)
	{
		NowVars.Reset();
	}
	else
	{
		FString NowFuncName;
		for (auto& Node : NowLuaStack)
		{
			if (Node->StackIndex == StackIndex)
				NowFuncName = Node->FuncInfo;
		}

		if (!LastShowVarFuncName.IsEmpty() && LastShowVarFuncName != NowFuncName)
			NowVars.Reset();
		LastShowVarFuncName = NowFuncName;
		UScriptDebuggerSetting::Get(IsDebugRemote)->GetStackVars(StackIndex, NowVars);

	}
	if (DebuggerVarTree.IsValid())
	{
		DebuggerVarTree.Pin()->RequestTreeRefresh();
	}
}

TSharedRef<ITableRow> SScriptDebugger::HandleFileTreeGenerateRow(FLuaFileTreeNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLuaFileTreeWidgetItem, OwnerTable, InNode);
}


void SScriptDebugger::HandleFileTreeGetChildren(FLuaFileTreeNode_Ref InNode, TArray<FLuaFileTreeNode_Ref>& OutChildren)
{
	InNode->GetAllChildren(OutChildren);
}


void SScriptDebugger::HandleFileTreeSelectionChanged(TSharedPtr<FLuaFileTreeNode> InNode, ESelectInfo::Type SelectInfo)
{
	if (InNode.IsValid() && !InNode->IsDir)
	{
		ShowCode(InNode->GetFilePath());
	}
}


void SScriptDebugger::HandleFileNodeExpansion(FLuaFileTreeNode_Ref InNode, bool bIsExpaned)
{

}


TSharedRef<ITableRow> SScriptDebugger::HandleCodeListGenerateRow(FCodeListNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SCodeWidgetItem, OwnerTable, InNode);
}


void SScriptDebugger::HandleCodeListSelectionChanged(TSharedPtr<FCodeListNode>, ESelectInfo::Type)
{

}


TSharedRef<ITableRow> SScriptDebugger::HandleStackListGenerateRow(FStackListNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SLuaStackWidgetItem, OwnerTable, InNode);
}


void SScriptDebugger::HandleStackListSelectionChanged(TSharedPtr<FStackListNode> InNode, ESelectInfo::Type)
{
	if (InNode.IsValid())
	{
		if (!InNode->FilePath.IsEmpty())
		{
			ShowCode(InNode->FilePath, InNode->Line);
		}
		ShowStackVars(InNode->StackIndex);
	}
}


TSharedRef<ITableRow> SScriptDebugger::HandleBreakPointListGenerateRow(FBreakPointNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SBreakPointWidgetItem, OwnerTable, InNode);
}


void SScriptDebugger::HandleBreakPointListSelectionChanged(TSharedPtr<FScriptBreakPointNode> InNode, ESelectInfo::Type)
{
	if (InNode.IsValid())
		ShowCode(InNode->FilePath, InNode->Line);
}

TSharedRef<ITableRow> SScriptDebugger::HandleVarsTreeGenerateRow(FDebuggerVarNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDebuggerVarTreeWidgetItem, OwnerTable, InNode);
}


void SScriptDebugger::HandleVarsTreeGetChildren(FDebuggerVarNode_Ref InNode, TArray<FDebuggerVarNode_Ref>& OutChildren)
{
	InNode->GetChildren(OutChildren);
}


void SScriptDebugger::HandleVarsTreeSelectionChanged(TSharedPtr<FScriptDebuggerVarNode>, ESelectInfo::Type)
{

}

void SScriptDebugger::RefreshLuaFileData()
{
	FString BaseDir = GetLuaSourceDir();
	FLuaFileTreeNode_Ref RootNode = MakeShareable(new FLuaFileTreeNode(true, FString(""), BaseDir));
	RootNode->FindChildren();
	RootNode->GetAllChildren(LuaFiles);
	const FText EmptyText = FText::FromString(TEXT(""));
	OnFileFilterTextChanged(EmptyText);
}


void SScriptDebugger::CleanDebugInfo()
{
	NowLuaStack.Reset();
	RefreshStackList();
}

FReply SScriptDebugger::DebugContinue()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		ShowStackVars(-1);
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
	}
	UScriptDebuggerSetting::Get(IsDebugRemote)->DebugContinue();
	return FReply::Handled();
}


FReply SScriptDebugger::DebugStepover()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
		UScriptDebuggerSetting::Get(IsDebugRemote)->StepOver();
	}
	return FReply::Handled();
}


FReply SScriptDebugger::DebugStepin()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
		UScriptDebuggerSetting::Get(IsDebugRemote)->StepIn();
	}
	return FReply::Handled();
}


FReply SScriptDebugger::DebugStepout()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
		UScriptDebuggerSetting::Get(IsDebugRemote)->StepOut();
	}
	return FReply::Handled();
}

void SScriptDebugger::DebugTabClose(TSharedRef<SDockTab> DockTab)
{
	UScriptDebuggerSetting::Get(false)->SetTabIsOpen(false);
	UScriptDebuggerSetting::Get(true)->SetTabIsOpen(false);
	DebugContinue();
	FSlateApplication::Get().UnregisterInputPreProcessor(ptr_HandleKeyDown);
	SaveDebuggerConfig();
	NowLuaCodeFilePath = "";
}


void SScriptDebugger::RegisterKeyDown()
{
	FSlateApplication::Get().RegisterInputPreProcessor(ptr_HandleKeyDown);
}


void SScriptDebugger::BeforeExit()
{
	SaveDebuggerConfig();
}

void SScriptDebugger::SaveDebuggerConfig()
{
	if (LuaCodeListPtr.IsValid())
	{
		float NowOffset = LuaCodeListPtr.Pin()->GetScrollOffset();
		LastTimeFileOffset.Add(NowLuaCodeFilePath, NowOffset);
	}
	UScriptDebuggerSetting::Get(IsDebugRemote)->LastTimeFileOffset = LastTimeFileOffset;
	UScriptDebuggerSetting::Get(IsDebugRemote)->RecentFilePath = RecentFilePath;
	UScriptDebuggerSetting::Get(IsDebugRemote)->RecentBreakPoint.Reset();
	for (FBreakPointNode_Ref &Node : BreakPointForView)
	{
		UScriptDebuggerSetting::Get(IsDebugRemote)->RecentBreakPoint.Add(*Node);
	}
	UScriptDebuggerSetting::Get(IsDebugRemote)->SaveConfig();
}

void SScriptDebugger::OnFileFilterTextChanged(const FText& InFilterText)
{
	if (InFilterText.IsEmpty())
		AfterFilterLuaFiles = LuaFiles;
	else
	{
		FString FileStr = InFilterText.ToString();
		FString FilePatternStr = ".*";
		for (int i = 0; i < FileStr.Len(); i++)
		{
			FilePatternStr += FileStr[i];
			FilePatternStr += ".*";
		}
		FRegexPattern FilePattern(FilePatternStr);
		AfterFilterLuaFiles.Reset();
		for (FLuaFileTreeNode_Ref& FileNode : LuaFiles)
		{
			FielterFileNode(FilePattern, FileNode);
		}
	}
	if (LuaFileTreePtr.IsValid())
		LuaFileTreePtr.Pin()->RequestTreeRefresh();
}


void SScriptDebugger::OnSearchLineChange(const FText& LineText)
{
	if (LineText.IsNumeric() && !NowLuaCodeFilePath.IsEmpty())
	{
		int32 LineNum = FCString::Atoi(*LineText.ToString());
		ShowCode(NowLuaCodeFilePath, LineNum);
	}
}

void SScriptDebugger::FielterFileNode(FRegexPattern& Pattern, FLuaFileTreeNode_Ref& FileNode)
{
	if (FileNode->IsDir)
	{
		TArray<FLuaFileTreeNode_Ref> OutChildren;
		FileNode->GetAllChildren(OutChildren);
		for (FLuaFileTreeNode_Ref& Node : OutChildren)
		{
			FielterFileNode(Pattern, Node);
		}
	}
	else
	{
		FRegexMatcher Matcher(Pattern, FileNode->FileName);
		if (Matcher.FindNext())
		{
			AfterFilterLuaFiles.Add(FileNode);
		}
	}
}


void SScriptDebugger::Tick(float Delta)
{
	IntervalToCheckFileChange += Delta;
	if (IntervalToCheckFileChange > 1.0f)
	{
		IntervalToCheckFileChange = 0;
		if (!NowLuaCodeFilePath.IsEmpty())
		{
			FDateTime ModifyTime = IFileManager::Get().GetTimeStamp(*NowLuaCodeFilePath);
			if (ModifyTime != ModifyTimeOfNowFile)
				ShowCode(NowLuaCodeFilePath);
		}
	}
}

void SScriptDebugger::AddMenuExtension(FMenuBuilder& Builder)
{
	//Builder.AddMenuEntry(FLuaDebuggerCommands::Get().OpenPluginWindow);
}

void SScriptDebugger::AddToolbarExtension(FToolBarBuilder& Builder)
{
	//Builder.AddToolBarButton(FLuaDebuggerCommands::Get().OpenPluginWindow);
}

#undef LOCTEXT_NAMESPACE

//IMPLEMENT_MODULE(SDebugger, LuaDebugger)

void FLuaFileTreeNode::FindChildren()
{
	if (IsDir)
	{
		TArray<FString> DirectoryNames;
		IFileManager::Get().FindFiles(DirectoryNames, *(BasePath / "*"), false, true);

		TArray<FString> NowDirNames;
		DirChildren.GenerateKeyArray(NowDirNames);
		for (auto& Name : NowDirNames)
		{
			if (!DirectoryNames.Contains(Name))
				DirChildren.Remove(Name);
		}

		for (auto& Name : DirectoryNames)
		{
			if (!DirChildren.Find(Name))
			{
				FLuaFileTreeNode_Ref NewOne = MakeShareable(new FLuaFileTreeNode(true, Name, BasePath / Name));
				DirChildren.Add(Name, NewOne);
			}
		}

		TArray<FString> FileNames;
		IFileManager::Get().FindFiles(FileNames, *(BasePath / "*"), true, false);

		TArray<FString> NowFileNames;
		FileChildren.GenerateKeyArray(NowFileNames);
		for (auto& Name : NowFileNames)
		{
			if (!FileNames.Contains(Name))
				FileChildren.Remove(Name);
		}

		for (auto& Name : FileNames)
		{
			if (!FileChildren.Find(Name))
			{
				if (FPaths::GetExtension(Name) == "lua")
				{
					FLuaFileTreeNode_Ref NewOne = MakeShareable(new FLuaFileTreeNode(false, Name, BasePath));
					FileChildren.Add(Name, NewOne);
				}
			}
		}
	}
}

void FLuaFileTreeNode::GetAllChildren(TArray<TSharedRef<FLuaFileTreeNode>>& OutArr)
{
	FindChildren();
	DirChildren.GenerateValueArray(OutArr);
	OutArr.StableSort([&](const TSharedRef<FLuaFileTreeNode>& a, const TSharedRef<FLuaFileTreeNode>& b) {return a->FileName < b->FileName; });
	TArray<TSharedRef<FLuaFileTreeNode>> FileChildrenArr;
	FileChildren.GenerateValueArray(FileChildrenArr);
	FileChildrenArr.StableSort([&](const TSharedRef<FLuaFileTreeNode>& a, const TSharedRef<FLuaFileTreeNode>& b) {return a->FileName < b->FileName; });
	OutArr.Append(FileChildrenArr);
}


void SCodeWidgetItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FCodeListNode_Ref Node)
{
	CodeNode = Node;
	static FSlateFontInfo CodeFont = FSlateFontInfo();
	CodeFont.Size = 24;
	STableRow<FCodeListNode_Ref>::Construct(STableRow<FCodeListNode_Ref>::FArguments(), InOwnerTableView);
	ChildSlot
		[
			// 			SNew(SBorder)
			// 			.Padding(FMargin(0.0f, 0.0f, 5.0f,0.0f))
			SNew(SBox)
			.MinDesiredHeight(20)
		.VAlign(VAlign_Center)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			[
				SNew(SBorder)
				.OnMouseButtonUp(this, &SCodeWidgetItem::OnRightClickBreakpoint)
		[
			SNew(SButton)
			.ForegroundColor(FSlateColor::UseForeground())
		.OnClicked(this, &SCodeWidgetItem::HandleClickBreakPoint)
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("Level.VisibleHighlightIcon16x"))
		.Visibility(this, &SCodeWidgetItem::BreakPointVisible)
		]
		]
			]
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			.MinDesiredWidth(40)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%d"), Node->Line)))
		]
		]
	+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Node->Code))
		]
		]
		];
}

EVisibility SCodeWidgetItem::BreakPointVisible() const
{
	return SScriptDebugger::Get()->HasBreakPoint(CodeNode->FilePath, CodeNode->Line) ? EVisibility::Visible : EVisibility::Hidden;
}

FReply SCodeWidgetItem::HandleClickBreakPoint()
{
	SScriptDebugger::Get()->ToggleBreakPoint(CodeNode->FilePath, CodeNode->Line);
	SScriptDebugger::Get()->SaveDebuggerConfig();
	return FReply::Handled();
}

FReply SCodeWidgetItem::OnRightClickBreakpoint(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (BreakPointVisible() == EVisibility::Visible)
	{
		const FVector2D& SummonLocation = MouseEvent.GetScreenSpacePosition();
		TSharedPtr<FScriptBreakPointNode> BreakPointNode = SScriptDebugger::Get()->GetViewBreakPoint(CodeNode->FilePath, CodeNode->Line);
		FText BreakCondition;
		if (BreakPointNode.IsValid())
			BreakCondition = BreakPointNode->HitCondition;
		TSharedPtr<SWidget> MenuContent =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SEditableTextBox)
				.HintText(FText::FromString("Condition"))
			.Text(BreakCondition)
			.OnTextCommitted(this, &SCodeWidgetItem::OnBreakConditionCommit)
			];

		if (MenuContent.IsValid())
		{
			FWidgetPath WidgetPath = MouseEvent.GetEventPath() != nullptr ? *MouseEvent.GetEventPath() : FWidgetPath();
			FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuContent.ToSharedRef(), SummonLocation, FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
		}


		return FReply::Handled();
	}
	else
		return FReply::Unhandled();
}

void SCodeWidgetItem::OnBreakConditionCommit(const FText& ConditionText, ETextCommit::Type CommitType)
{
	TSharedPtr<FScriptBreakPointNode> BreakPointNode = SScriptDebugger::Get()->GetViewBreakPoint(CodeNode->FilePath, CodeNode->Line);
	if (BreakPointNode.IsValid())
	{
		BreakPointNode->HitCondition = ConditionText;
		UScriptDebuggerSetting::Get(false)->BreakConditionChange();
		UScriptDebuggerSetting::Get(true)->BreakConditionChange();
	}

}

void SLuaStackWidgetItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FStackListNode_Ref Node)
{
	STableRow<FStackListNode_Ref>::Construct(STableRow<FStackListNode_Ref>::FArguments(), InOwnerTableView);
	StackNode = Node;
	ChildSlot
		[
			SNew(SBox)
			.MinDesiredHeight(20)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Node->Code))
		]
		];
}

FName SDebuggerVarTreeWidgetItem::Col_Name = "key";
FName SDebuggerVarTreeWidgetItem::Col_Value = "value";

TSharedRef<SWidget> SDebuggerVarTreeWidgetItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == Col_Name)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text_Lambda([&]()->FText {return VarInfoNode->Name; })
			];
	}
	else
	{
		return SNew(STextBlock)
			.Text_Lambda([&]()->FText {return VarInfoNode->Value; })
			;
	}
}


void SScriptDebugger::FHandleKeyDown::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> MyCursor)
{
	SScriptDebugger::Get()->Tick(DeltaTime);
}

bool SScriptDebugger::FHandleKeyDown::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::F5)
	{
		SScriptDebugger::Get()->DebugContinue();
		return true;
	}
	else if (InKeyEvent.GetKey() == EKeys::F10)
	{
		SScriptDebugger::Get()->DebugStepover();
		return true;
	}
	else if (InKeyEvent.GetKey() == EKeys::F11)
	{
		if (InKeyEvent.IsLeftShiftDown() || InKeyEvent.IsRightShiftDown())
			SScriptDebugger::Get()->DebugStepout();
		else
			SScriptDebugger::Get()->DebugStepin();
		return true;
	}
	return false;
}

void SBreakPointWidgetItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FBreakPointNode_Ref BreakPointNode)
{
	STableRow<FBreakPointNode_Ref>::Construct(STableRow<FBreakPointNode_Ref>::FArguments(), InOwnerTableView);

	FString LuaSourceDir = SScriptDebugger::Get()->GetLuaSourceDir();
	ChildSlot
		[
			SNew(SBox)
			.MinDesiredHeight(20)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(FString::Printf(TEXT("%s@Line %d"), *BreakPointNode->FilePath.Mid(LuaSourceDir.Len() + 1), BreakPointNode->Line)))
		]
		];
}
