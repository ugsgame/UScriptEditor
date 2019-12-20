// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VarNode.h"
#include "STableRow.h"
#include "STreeView.h"

#include "UScriptDebuggerSetting.h"


class SVarTreeWidgetItem :public SMultiColumnTableRow<TSharedRef<FVarWatcherNode>>
{
public:
	static FName Col_Name;
	static FName Col_Value;
	static FName Col_Type;
	TSharedPtr<FVarWatcherNode> VarInfoNode;
	SLATE_BEGIN_ARGS(SVarTreeWidgetItem)
		:_VarInfoToVisualize()
	{}
		SLATE_ARGUMENT(TSharedPtr<FVarWatcherNode>, VarInfoToVisualize)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		VarInfoNode = InArgs._VarInfoToVisualize;
		SMultiColumnTableRow<TSharedRef<FVarWatcherNode>>::Construct(SMultiColumnTableRow< TSharedRef<FVarWatcherNode> >::FArguments().Padding(1), InOwnerTableView);
	}
		virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override;
};

class SVarWatcher : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVarWatcher) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
public:
	static SVarWatcher* Ptr;
	static SVarWatcher* Get() { return Ptr; }
	/** IModuleInterface implementation */
	typedef STreeView<TSharedRef<FVarWatcherNode>> SVarsTree;
	
	TWeakPtr<SVarsTree> VarTreePtr;
	bool bNeedTickTreeView;
	bool bNeedUpdateData;
	bool bShowFunction;

	TSharedRef<ITableRow> HandleVarTreeGenerateRow(TSharedRef<FVarWatcherNode> InVarNode, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleVarTreeGetChildren(TSharedRef<FVarWatcherNode> InVarNode, TArray<TSharedRef<FVarWatcherNode>>& OutChildren);
	void HandleVarTreeSelectionChanged(TSharedPtr<FVarWatcherNode>, ESelectInfo::Type /*SelectInfo*/);
	void HandleNodeExpansion(TSharedRef<FVarWatcherNode> VarNode, bool bIsExpaned);

	virtual void StartupModule() ;
	virtual void ShutdownModule() ;
	
	void Update(float Delta);

	void RefreshVarTree();

private:

	void OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

};