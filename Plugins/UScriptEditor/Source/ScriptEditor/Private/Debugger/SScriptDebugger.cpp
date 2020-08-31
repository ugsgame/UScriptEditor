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
#include "ScriptHelperBPFunLib.h"

#include "ProjectEditor/CodeProjectItem.h"
#include "ProjectEditor/SProjectTreeEditor.h"
#include "UScriptRemoteDebuggerSetting.h"


#define LOCTEXT_NAMESPACE "SScriptDebugger"

void FScriptDebuggerVarNode::GetChildren(TArray<TSharedRef<FScriptDebuggerVarNode>>& OutChildren)
{
	if (!SScriptDebugger::Get()->IsDebugRemote)
	{
		UScriptDebuggerSetting::Get()->GetVarsChildren(*this);
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->GetVarsChildren(*this);
	}

	NodeChildren.GenerateValueArray(OutChildren);
}

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
	LastTimeFileOffset = UScriptDebuggerSetting::Get()->LastTimeFileOffset;
	RecentFilePath = UScriptDebuggerSetting::Get()->RecentFilePath;

	/*
	for (FScriptBreakPointNode &Node : UScriptDebuggerSetting::Get()->RecentBreakPoint)
	{
		TSet<int32>& Set = EnableBreakPoint.FindOrAdd(Node.FilePath);
		Set.Add(Node.Line);
		FBreakPointNode_Ref NewNode = MakeShareable(new FScriptBreakPointNode(Node.Line, Node.FilePath));
		NewNode->HitCondition = Node.HitCondition;
		BreakPointForView.Add(NewNode);
	}
	*/

	OnSpawnPluginTab(FSpawnTabArgs(TSharedPtr<SWindow>(), FTabId()));
}

void SScriptDebugger::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	UScriptDebuggerSetting::Get()->SetTabIsOpen(true);

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

			+ SHeaderRow::Column(SDebuggerVarTreeWidgetItem::Col_Type)
			.DefaultLabel(LOCTEXT("VarType1", "Type"))
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
		.ListItemsSource(&FScriptEditor::Get()->BreakPointForView)
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
	UpdateDebugState();
}

void SScriptDebugger::ToggleRemoteDebug(const ECheckBoxState NewState)
{
	IsDebugRemote = (NewState == ECheckBoxState::Checked) ? true : false;
	UpdateDebugState();
}

void SScriptDebugger::UpdateDebugState()
{
	if (IsDebugRun)
	{
		if (IsDebugRemote)
		{
			UScriptRemoteDebuggerSetting::Get()->CreateHookServer();
			UScriptDebuggerSetting::Get()->UnBindDebugState();
		}
		else
		{
			UScriptRemoteDebuggerSetting::Get()->DestroyHookServer();
			UScriptDebuggerSetting::Get()->BindDebugState();

			CleanDebugInfo();
		}
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->DestroyHookServer();
		UScriptDebuggerSetting::Get()->UnBindDebugState();
	}
	IsEnterDebugMode = false;
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
	FString ScriptSourceDir = UScriptHelperBPFunLib::ScriptSourceDir();
	FPaths::NormalizeDirectoryName(ScriptSourceDir);
	return  ScriptSourceDir;
}

void SScriptDebugger::EnterDebug(const FString& LuaFilePath, int32 Line)
{
	ShowCode(LuaFilePath, Line);
	UScriptDebuggerSetting::Get()->HittingPoint.Line = Line;
	UScriptDebuggerSetting::Get()->HittingPoint.FilePath = LuaFilePath;

	if (!IsDebugRemote)
	{
		ShowStackVars(0);
		IsEnterDebugMode = true;
		FSlateApplication::Get().EnterDebuggingMode();
	}
	else
	{
		IsEnterDebugMode = true;
	}
}


void SScriptDebugger::SetStackData(TArray<TTuple<int32, int32, FString, FString>>& StackInfos)
{
	NowLuaStack.Reset();
	for (auto StackItem : StackInfos)
	{
		FStackListNode_Ref NewNode = MakeShareable(new FStackListNode(StackItem.Get<0>(), StackItem.Get<1>(), StackItem.Get<2>(), StackItem.Get<3>()));
		NowLuaStack.Add(NewNode);
	}
	RefreshStackList();
}

void SScriptDebugger::ShowCode(const FString& FilePath, int32 Line /*= 0*/)
{
	//TODO
	UCodeProjectItem* ScriptProject = FScriptEditor::Get()->GetScriptProjectBeingEdited();
	UCodeProjectItem* CodeItem = ScriptProject->FindChild(FilePath);

	if (CodeItem)
	{
		SProjectTreeEditor::Get()->ExpanedScriptItem(CodeItem);
		FScriptEditor::Get()->OpenFileAndGotoLine(CodeItem, Line);
	}
}


void SScriptDebugger::ShowStackVars(int32 StackIndex)
{
	if (!IsDebugRemote)
	{
		NowVars.Reset();
		UScriptDebuggerSetting::Get()->GetStackVars(StackIndex, NowVars);
		if (DebuggerVarTree.IsValid())
		{
			DebuggerVarTree.Pin()->RequestTreeRefresh();
		}
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->SendReqStack(StackIndex);
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

TSharedRef<ITableRow> SScriptDebugger::HandleVarsTreeGenerateRow(FScriptDebuggerVarNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SDebuggerVarTreeWidgetItem, OwnerTable, InNode);
}


void SScriptDebugger::HandleVarsTreeGetChildren(FScriptDebuggerVarNode_Ref InNode, TArray<FScriptDebuggerVarNode_Ref>& OutChildren)
{
	InNode->GetChildren(OutChildren);
}


void SScriptDebugger::HandleVarsTreeSelectionChanged(TSharedPtr<FScriptDebuggerVarNode> InNode, ESelectInfo::Type SelectType)
{
	//UE_LOG(LogTemp, Log, TEXT("Remote VarsTreeSelection %s"), *InNode->ToString());
}

void SScriptDebugger::RemoteRefreshVars(TArray<FScriptDebuggerVarNode_Ref>& VarList)
{
	NowVars.Reset();
	NowVars.Append(VarList);
	if (DebuggerVarTree.IsValid())
	{
		DebuggerVarTree.Pin()->RequestTreeRefresh();
	}
}

void SScriptDebugger::UpdateBreakPoints()
{
	if (IsDebugRemote)
		UScriptRemoteDebuggerSetting::Get()->SendBreakPoints();
}


void SScriptDebugger::DebugContinue()
{
	IsEnterDebugMode = false;
	if (!IsDebugRemote)
	{
		FSlateApplication::Get().LeaveDebuggingMode();
		UScriptDebuggerSetting::Get()->Continue();
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->SendContinue();
	}
}


void SScriptDebugger::DebugStepover()
{
	IsEnterDebugMode = false;
	if (!IsDebugRemote)
	{
		FSlateApplication::Get().LeaveDebuggingMode();
		UScriptDebuggerSetting::Get()->StepOver();
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->SendStepOver();
	}
}


void SScriptDebugger::DebugStepin()
{
	IsEnterDebugMode = false;
	if (!IsDebugRemote)
	{
		FSlateApplication::Get().LeaveDebuggingMode();
		UScriptDebuggerSetting::Get()->StepIn();
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->SendStepIn();
	}
}


void SScriptDebugger::DebugStepout()
{
	IsEnterDebugMode = false;
	if (!IsDebugRemote)
	{
		FSlateApplication::Get().LeaveDebuggingMode();
		UScriptDebuggerSetting::Get()->StepOut();
	}
	else
	{
		UScriptRemoteDebuggerSetting::Get()->SendStepOut();
	}
}

void SScriptDebugger::DebugTabClose(TSharedRef<SDockTab> DockTab)
{
	UScriptDebuggerSetting::Get()->SetTabIsOpen(false);

	if (IsEnterDebugMode)
	{
		FSlateApplication::Get().LeaveDebuggingMode();
		IsEnterDebugMode = false;
	}

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

void SScriptDebugger::CleanDebugInfo()
{
	//she is IsEnterDebugMode false anywhere
	IsEnterDebugMode = false;

	NowLuaStack.Reset();
	if (LuaStackListPtr.IsValid())
	{
		LuaStackListPtr.Pin()->RequestListRefresh();
	}
	NowVars.Reset();
	if (DebuggerVarTree.IsValid())
	{
		DebuggerVarTree.Pin()->RequestTreeRefresh();
	}
}

void SScriptDebugger::SaveDebuggerConfig()
{
	if (IsDebugerClosed)return;

	// 	if (LuaCodeListPtr.IsValid())
	// 	{
	// 		float NowOffset = LuaCodeListPtr.Pin()->GetScrollOffset();
	// 		LastTimeFileOffset.Add(NowLuaCodeFilePath, NowOffset);
	// 	}

	UScriptDebuggerSetting::Get()->LastTimeFileOffset = LastTimeFileOffset;
	UScriptDebuggerSetting::Get()->RecentFilePath = RecentFilePath;

}


void SScriptDebugger::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	//UScriptRemoteDebuggerSetting::Get()->SlateTick(InDeltaTime);
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
FName SDebuggerVarTreeWidgetItem::Col_Type = "Type";

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
				.Text_Lambda([&]()->FText {return VarInfoNode->VarName; })
			];
	}
	else if (ColumnName == Col_Value)
	{
		if (VarInfoNode->KindType == (int32)EScriptUEKindType::T_TList || VarInfoNode->KindType == (int32)EScriptUEKindType::T_TDict)
			return SNullWidget::NullWidget;

		return SNew(STextBlock)
			.Text_Lambda([&]()->FText {return VarInfoNode->VarValue; })
			;
	}
	else
	{
		return SNew(STextBlock)
			.Text_Lambda([&]()->FText {return VarInfoNode->VarType; })
			;
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




