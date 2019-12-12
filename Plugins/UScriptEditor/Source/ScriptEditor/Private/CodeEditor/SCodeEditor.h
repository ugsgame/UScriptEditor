// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SCodeEditableText;
class SScrollBar;
class SScrollBox;

class UCodeProjectItem;

struct FCodeLineNode
{
	FCodeLineNode(int32 _Line, FString _FilePath)
		:FilePath(_FilePath), Line(_Line), HasBreakPoint(false)
	{}
	FString FilePath;
	int32 Line;
	bool HasBreakPoint;
};

using FCodeLineNode_Ptr = TSharedPtr<FCodeLineNode>;

class SCodeLineItem :public STableRow<FCodeLineNode_Ptr>
{
public:

	SLATE_BEGIN_ARGS(SCodeLineItem)
	{}
	SLATE_END_ARGS()

	TSharedPtr<FCodeLineNode> CodeLine;
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FCodeLineNode_Ptr Line);

	void OnClickBreakPoint(const ECheckBoxState NewCheckedState);
	void OnBreakConditionCommit(const FText& ConditionText, ETextCommit::Type CommitType);

protected:

	TSharedPtr<class SCheckBox> BreakPointCheckBox;
};

class SCodeEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCodeEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UCodeProjectItem* InCodeProjectItem);

	//
	bool Save() const;
	bool CanSave() const;
	void Browser() const;
	//
	//
	int32 GetLineCount() const;
	UCodeProjectItem* GetCodeProjectItem()const;
	void GotoLineAndColumn(int32 LineNumber, int32 ColumnNumber);
	FText GetLineAndColumn() const;
	//
	virtual void OnClose();

protected:
	void OnTextChanged(const FText& NewText);
	void OnTextCommitted(const FText &NewText, ETextCommit::Type CommitInfo);
	void OnVerticalScroll(float Offset);
	void SetLineCountList(const int32 Count);
	void OnSelectedLineCounterItem(FCodeLineNode_Ptr Item, ESelectInfo::Type SelectInfo);
	void OnDoubleClickLineCounterItem(FCodeLineNode_Ptr Item);
	TSharedRef<ITableRow>OnGenerateLineCounter(FCodeLineNode_Ptr Item, const TSharedRef<STableViewBase>&OwnerTable);

protected:
	UCodeProjectItem* CodeProjectItem;

	TArray<TSharedPtr<FCodeLineNode>>LineCount;

	TSharedPtr<SScrollBar> HorizontalScrollbar;
	TSharedPtr<SScrollBar> VerticalScrollbar;
	TSharedPtr<SScrollBox> VS_SCROLL_BOX;

	TSharedPtr<class SCodeEditableText> CodeEditableText;
	TSharedPtr<SListView<FCodeLineNode_Ptr>>LineCounter;

	mutable bool bDirty;
};

