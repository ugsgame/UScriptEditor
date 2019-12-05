// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SScriptDebugger.h"
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

#include "ScriptEditor.h"


#define LOCTEXT_NAMESPACE "SScriptDebugger"

SScriptDebugger* SScriptDebugger::Ptr;

void SScriptDebugger::Construct(const FArguments& InArgs)
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	Ptr = this;

	IsDebugRun = true;
	IsDebugRemote = false;
	IsEnterDebugMode = false;
	IsDebugerClosed = false;

	IntervalToCheckFileChange = 0;
	ptr_HandleKeyDown = MakeShareable(new FHandleKeyDown());
	StackListState = EStackListState::CallStack;
	LastTimeFileOffset = UScriptDebuggerSetting::Get(IsDebugRemote)->LastTimeFileOffset;
	RecentFilePath = UScriptDebuggerSetting::Get(IsDebugRemote)->RecentFilePath;

	/*
	for (FScriptBreakPointNode &Node : UScriptDebuggerSetting::Get(IsDebugRemote)->RecentBreakPoint)
	{
		TSet<int32>& Set = EnableBreakPoint.FindOrAdd(Node.FilePath);
		Set.Add(Node.Line);
		FBreakPointNode_Ref NewNode = MakeShareable(new FScriptBreakPointNode(Node.Line, Node.FilePath));
		NewNode->HitCondition = Node.HitCondition;
		BreakPointForView.Add(NewNode);
	}
	*/

	FCoreDelegates::OnPreExit.AddRaw(this, &SScriptDebugger::BeforeExit);

	OnSpawnPluginTab(FSpawnTabArgs(TSharedPtr<SWindow>(), FTabId()));
}

void SScriptDebugger::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	UScriptDebuggerSetting::Get(false)->SetTabIsOpen(true);
	UScriptDebuggerSetting::Get(true)->SetTabIsOpen(true);
	SyncState();
	FSlateApplication::Get().RegisterInputPreProcessor(ptr_HandleKeyDown);

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
					.Value(1.f)
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
										.ListItemsSource(& FScriptEditor::Get()->BreakPointForView)
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
}

FText SScriptDebugger::GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine)
{
	TSharedPtr<FScriptBreakPointNode> BreakPointNode = FScriptEditor::Get()->GetViewBreakPoint(FilePath, CodeLine);
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

	UScriptDebuggerSetting::Get(true)->UpdateBreakPoint(FScriptEditor::Get()->EnableBreakPoint);
	UScriptDebuggerSetting::Get(false)->UpdateBreakPoint(FScriptEditor::Get()->EnableBreakPoint);
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
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / "Scripts");
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
	//TODO
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

void SScriptDebugger::CleanDebugInfo()
{
	NowLuaStack.Reset();
	RefreshStackList();
}

void SScriptDebugger::DebugContinue()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		ShowStackVars(-1);
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
	}
	UScriptDebuggerSetting::Get(IsDebugRemote)->DebugContinue();
}


void SScriptDebugger::DebugStepover()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
		UScriptDebuggerSetting::Get(IsDebugRemote)->StepOver();
	}
}


void SScriptDebugger::DebugStepin()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
		UScriptDebuggerSetting::Get(IsDebugRemote)->StepIn();
	}
}


void SScriptDebugger::DebugStepout()
{
	if (IsEnterDebugMode)
	{
		CleanDebugInfo();
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
		UScriptDebuggerSetting::Get(IsDebugRemote)->StepOut();
	}
}

void SScriptDebugger::DebugTabClose(TSharedRef<SDockTab> DockTab)
{
	UScriptDebuggerSetting::Get(false)->SetTabIsOpen(false);
	UScriptDebuggerSetting::Get(true)->SetTabIsOpen(false);
	DebugContinue();
	FSlateApplication::Get().UnregisterInputPreProcessor(ptr_HandleKeyDown);
	SaveDebuggerConfig();
	NowLuaCodeFilePath = "";

	IsDebugerClosed = true;
	Ptr = nullptr;
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
	if(IsDebugerClosed)return;
	
// 	if (LuaCodeListPtr.IsValid())
// 	{
// 		float NowOffset = LuaCodeListPtr.Pin()->GetScrollOffset();
// 		LastTimeFileOffset.Add(NowLuaCodeFilePath, NowOffset);
// 	}

	UScriptDebuggerSetting::Get(IsDebugRemote)->LastTimeFileOffset = LastTimeFileOffset;
	UScriptDebuggerSetting::Get(IsDebugRemote)->RecentFilePath = RecentFilePath;
	UScriptDebuggerSetting::Get(IsDebugRemote)->RecentBreakPoint.Reset();

	for (FBreakPointNode_Ref &Node : FScriptEditor::Get()->BreakPointForView)
	{
		UScriptDebuggerSetting::Get(IsDebugRemote)->RecentBreakPoint.Add(*Node);
	}
	UScriptDebuggerSetting::Get(IsDebugRemote)->SaveConfig();

}


void SScriptDebugger::Update(float Delta)
{
	if (IsDebugerClosed)return;
	//TODO:Not here, Move to CodeEditor
	/*
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
	*/
}

#undef LOCTEXT_NAMESPACE


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
			.Text_Lambda([&]()->FText {return VarInfoNode->Value; });
	}
}


void SScriptDebugger::FHandleKeyDown::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> MyCursor)
{
	if (SScriptDebugger::Get())
	{
		SScriptDebugger::Get()->Update(DeltaTime);
	}
}

bool SScriptDebugger::FHandleKeyDown::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	/*
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
	*/
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
