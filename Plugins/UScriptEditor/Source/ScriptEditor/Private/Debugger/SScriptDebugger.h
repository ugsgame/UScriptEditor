// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SCompoundWidget.h"
#include "STableRow.h"
#include "STreeView.h"
#include "UScriptDebuggerSetting.h"
#include "IInputProcessor.h"
#include "Regex.h"


struct FStackListNode
{
	int32 StackIndex;
	int32 Line;
	FString FilePath;
	FString FuncInfo;
	FString Code;

	FStackListNode(int32 _StackIndex, int32 _Line, FString _FilePath, const FString& _FuncInfo)
		:StackIndex(_StackIndex), Line(_Line), FilePath(_FilePath), FuncInfo(_FuncInfo)
	{
		Code = FString::Printf(TEXT("%s::%s line[%d]"), *FPaths::GetCleanFilename(FilePath), *FuncInfo, Line);
	}
};

using FStackListNode_Ref = TSharedRef<FStackListNode>;
using SLuaStackList = SListView<FStackListNode_Ref>;

class SLuaStackWidgetItem :public STableRow<FStackListNode_Ref>
{
public:
	TSharedPtr<FStackListNode> StackNode;
	SLATE_BEGIN_ARGS(SLuaStackWidgetItem)
	{}
	SLATE_END_ARGS()
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FStackListNode_Ref Node);

};

class SDebuggerVarTreeWidgetItem :public SMultiColumnTableRow<TSharedRef<FScriptDebuggerVarNode>>
{
public:
	static FName Col_Name;
	static FName Col_Value;
	static FName Col_Type;
	TSharedPtr<FScriptDebuggerVarNode> VarInfoNode;
	SLATE_BEGIN_ARGS(SDebuggerVarTreeWidgetItem)
	{}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FScriptDebuggerVarNode_Ref Node)
	{
		VarInfoNode = Node;
		SMultiColumnTableRow<TSharedRef<FScriptDebuggerVarNode>>::Construct(SMultiColumnTableRow< TSharedRef<FScriptDebuggerVarNode> >::FArguments().Padding(1), InOwnerTableView);
	}
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
};

using FBreakPointNode_Ref = TSharedRef<FScriptBreakPointNode>;
using SBreakPointList = SListView<FBreakPointNode_Ref>;

class SBreakPointWidgetItem :public STableRow<FBreakPointNode_Ref>
{
public:
	TSharedPtr<FScriptBreakPointNode> Node;
	SLATE_BEGIN_ARGS(SBreakPointWidgetItem)
	{}
	SLATE_END_ARGS()
		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FBreakPointNode_Ref Node);

};

enum class EStackListState : uint8
{
	CallStack,
	BreakPoints,
};


class SScriptDebugger : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScriptDebugger) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
public:
	static SScriptDebugger* Ptr;
	static SScriptDebugger* Get() { return Ptr; }

	FDateTime ModifyTimeOfNowFile;
	EStackListState StackListState;
	TArray<FStackListNode_Ref> NowLuaStack;
	TWeakPtr<SLuaStackList> LuaStackListPtr;

	TWeakPtr<SBreakPointList> BreakPointListPtr;

	UPROPERTY()
	FString NowLuaCodeFilePath;
	UPROPERTY()
	TArray<FScriptDebuggerVarNode_Ref> NowVars;
	UPROPERTY()
	TWeakPtr<SDebuggerVarTree> DebuggerVarTree;
	UPROPERTY()
	FString RecentFilePath;

	bool IsDebugRun;
	bool IsDebugRemote;
	bool IsEnterDebugMode;
	bool IsDebugerClosed;

	FString LastShowVarFuncName;
	TMap<FString, float> LastTimeFileOffset;

	FText GetBreakPointHitConditionText(FString& FilePath, int32 CodeLine);
	void ToggleStartDebug(const ECheckBoxState NewState);
	void ToggleRemoteDebug(const ECheckBoxState NewState);
	void ToggleStackListState(const ECheckBoxState NewState, EStackListState State);
	void BreakPointChange();
	void DebugStateChange();
	void RemoteStateChange();
	void SyncState();
	void RefreshStackList();
	static FString GetLuaSourceDir();
	void EnterDebug(const FString& LuaFilePath, int32 Line);
	void SetStackData(TArray<TTuple<int32, int32, FString, FString>>& StackInfos);
	void ShowCode(const FString& FilePath, int32 Line = 0);
	void ShowStackVars(int32 StackIndex);

	TSharedRef<ITableRow> HandleStackListGenerateRow(FStackListNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleStackListSelectionChanged(TSharedPtr<FStackListNode>, ESelectInfo::Type);

	TSharedRef<ITableRow> HandleBreakPointListGenerateRow(FBreakPointNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleBreakPointListSelectionChanged(TSharedPtr<FScriptBreakPointNode>, ESelectInfo::Type);

	TSharedRef<ITableRow> HandleVarsTreeGenerateRow(FScriptDebuggerVarNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleVarsTreeGetChildren(FScriptDebuggerVarNode_Ref InNode, TArray<FScriptDebuggerVarNode_Ref>& OutChildren);
	void HandleVarsTreeSelectionChanged(TSharedPtr<FScriptDebuggerVarNode>, ESelectInfo::Type);

	void CleanDebugInfo();
	void DebugContinue();
	void DebugStepover();
	void DebugStepin();
	void DebugStepout();
	void DebugTabClose(TSharedRef<SDockTab> DockTab);
	void RegisterKeyDown();
	void ClearStackInfo();
	void SaveDebuggerConfig();

	class FHandleKeyDown :public IInputProcessor
	{
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);

	};
	TSharedPtr<FHandleKeyDown> ptr_HandleKeyDown;

	float IntervalToCheckFileChange;
	void Update(float Delta);
private:

	void OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
};