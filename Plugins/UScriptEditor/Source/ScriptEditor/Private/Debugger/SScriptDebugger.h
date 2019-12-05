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
	FStackListNode(FString _Code, int32 _Line, FString _FilePath, int32 _StackIndex, const FString& _FuncInfo)
		:FilePath(_FilePath), Code(_Code), Line(_Line), StackIndex(_StackIndex), FuncInfo(_FuncInfo)
	{}
	FString FilePath;
	FString Code;
	int32 Line;
	int32 StackIndex;
	FString FuncInfo;
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
	TSharedPtr<FScriptDebuggerVarNode> VarInfoNode;
	SLATE_BEGIN_ARGS(SDebuggerVarTreeWidgetItem)
	{}
	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FDebuggerVarNode_Ref Node)
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

// struct ViewCodeHistory
// {
// 	int32 NowHeadIndex;
// 	int32 NowIndex;
// 	struct ViewAddr
// 	{
// 		FString FilePath;
// 		int32 Line;
// 		bool operator==(const ViewAddr& Right)
// 		{
// 			return FilePath == Right.FilePath && Line == Right.Line;
// 		}
// 	};
// 
// 	void Append(ViewAddr NewAddr)
// 	{
// 		if (NowIndex != NowHeadIndex)
// 		{
// 			NowHeadIndex = NowIndex;
// 		}
// 		NowHeadIndex++;
// // 		History.ResizeTo(NowHeadIndex + 1);
// 
// 	}
// 	TArray<ViewAddr> History;
// };

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
	TArray<FDebuggerVarNode_Ref> NowVars;
	UPROPERTY()
	TWeakPtr<SDebuggerVarTree> DebuggerVarTree;
	UPROPERTY()
	FString RecentFilePath;

	bool IsDebugRun;
	bool IsDebugRemote;
	bool IsEnterDebugMode;

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
	void SetStackData(const TArray<FString>& Content, const TArray<int32>& Lines, const TArray<FString>& FilePaths, const TArray<int32>& StackIndex, const TArray<FString>& FuncInfos);
	void ShowCode(const FString& FilePath, int32 Line = 0);
	void ShowStackVars(int32 StackIndex);

	TSharedRef<ITableRow> HandleStackListGenerateRow(FStackListNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleStackListSelectionChanged(TSharedPtr<FStackListNode>, ESelectInfo::Type);

	TSharedRef<ITableRow> HandleBreakPointListGenerateRow(FBreakPointNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleBreakPointListSelectionChanged(TSharedPtr<FScriptBreakPointNode>, ESelectInfo::Type);

	TSharedRef<ITableRow> HandleVarsTreeGenerateRow(FDebuggerVarNode_Ref InNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleVarsTreeGetChildren(FDebuggerVarNode_Ref InNode, TArray<FDebuggerVarNode_Ref>& OutChildren);
	void HandleVarsTreeSelectionChanged(TSharedPtr<FScriptDebuggerVarNode>, ESelectInfo::Type);

	void CleanDebugInfo();
	void DebugContinue();
	void DebugStepover();
	void DebugStepin();
	void DebugStepout();
	void DebugTabClose(TSharedRef<SDockTab> DockTab);
	void RegisterKeyDown();
	void BeforeExit();
	void SaveDebuggerConfig();

	class FHandleKeyDown :public IInputProcessor
	{
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);
		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);

	};
	TSharedPtr<FHandleKeyDown> ptr_HandleKeyDown;

	float IntervalToCheckFileChange;
	void Tick(float Delta);
private:

	void OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
};