// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

#include "UScriptDebuggerSetting.h"
#include "Widgets/Input/SComboBox.h"

class SCodeEditableText;
class SScrollBar;
class SScrollBox;

class UScriptProjectItem;

struct FCodeLineNode
{
	FCodeLineNode(int32 _Line, FString _FilePath)
		:FilePath(_FilePath), Line(_Line), HasBreakPoint(false)
	{}

	bool IsHit()
	{
		return (Line == UScriptDebuggerSetting::Get()->HittingPoint.Line && FilePath == UScriptDebuggerSetting::Get()->HittingPoint.FilePath);
	};

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

	ECheckBoxState BreakPointIsChecked()const;
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

	void Construct(const FArguments& InArgs, class UScriptProjectItem* InCodeProjectItem);

	//
	bool Save() const;
	bool CanSave() const;
	bool Reload();
	void Browser() const;
	//
	//
	int32 GetLineCount() const;
	UScriptProjectItem* GetCodeProjectItem()const;
	void GotoLineAndColumn(int32 LineNumber, int32 ColumnNumber);
	FText GetLineAndColumn() const;
	FText GetReferenceInfo() const;
	//
	virtual void OnClose();

protected:
	void CheckReferences();
	void OnInvokedSearch();
	void OnAdvanceAutoComplete(const FString &Search);
	void OnAutoComplete(const FString &Results);
	void OnTextChanged(const FText& NewText);
	void OnTextCommitted(const FText &NewText, ETextCommit::Type CommitInfo);
	void OnVerticalScroll(float Offset);
	bool IsCodeEditable()const;
	void SetLineCountList(const int32 Count);
	void OnSelectedLineCounterItem(FCodeLineNode_Ptr Item, ESelectInfo::Type SelectInfo);
	void OnDoubleClickLineCounterItem(FCodeLineNode_Ptr Item);
	TSharedRef<ITableRow>OnGenerateLineCounter(FCodeLineNode_Ptr Item, const TSharedRef<STableViewBase>&OwnerTable);

	TSharedRef<SWidget> OnGenerateTagSourcesComboBox(TSharedPtr<FName> InItem);
	void OnReferenceSelectionChanged(TSharedPtr<FName> InItem, ESelectInfo::Type InSeletionInfo);
	FText CreateTagSourcesComboBoxContent() const;

	void OnSearchTextChanged(const FText& InFilterText);
	void OnSearchTextCommitted(const FText& InText, ETextCommit::Type CommitInfo);

	bool GetBlueprintClassParents(const UClass* InClass, TArray<UBlueprint*>& OutBlueprintParents);
	bool GetNativeClassParents(const UClass* InClass, TArray<UClass*>& OutNativeClassParents);
protected:
	UScriptProjectItem* CodeProjectItem;

	TArray<TSharedPtr<FCodeLineNode>>LineCount;

	TSharedPtr<SScrollBar> HorizontalScrollbar;
	TSharedPtr<SScrollBar> VerticalScrollbar;

	TSharedPtr<SScrollBox> VS_SCROLL_BOX;

	TSharedPtr<class SCodeEditableText> CodeEditableText;
	TSharedPtr<class SSearchBox> SearchTextBox;
	TSharedPtr<class SComboBox<TSharedPtr<FName>>> ReferenceComboBox;
	TSharedPtr<class SListView<FCodeLineNode_Ptr>> LineCounter;

	TArray<TSharedPtr<FName>> ReferenceItems;
	TArray<FScriptReferenceInfo> ReferenceInfoes;

	mutable bool bDirty;
};

