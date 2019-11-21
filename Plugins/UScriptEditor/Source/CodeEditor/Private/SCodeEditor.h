// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class SCodeEditableText;
class SScrollBar;

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
	void GotoLineAndColumn(int32 LineNumber, int32 ColumnNumber);
	//

protected:

	void OnTextChanged(const FText& NewText);
	void OnTextCommitted(const FText &NewText, ETextCommit::Type CommitInfo);
	void OnVerticalScroll(float Offset);
	void SetLineCountList(const int32 Count);
	void OnSelectedLineCounterItem(TSharedPtr<FString>Item, ESelectInfo::Type SelectInfo);
	void OnDoubleClickLineCounterItem(TSharedPtr<FString>Item);
	TSharedRef<ITableRow>OnGenerateLineCounter(TSharedPtr<FString>Item, const TSharedRef<STableViewBase>&OwnerTable);

protected:
	class UCodeProjectItem* CodeProjectItem;

	TArray<TSharedPtr<FString>>LineCount;

	TSharedPtr<SScrollBar> HorizontalScrollbar;
	TSharedPtr<SScrollBar> VerticalScrollbar;
	TSharedPtr<SScrollBox> VS_SCROLL_BOX;

	TSharedPtr<class SCodeEditableText> CodeEditableText;
	TSharedPtr<SListView<TSharedPtr<FString>>>LineCounter;

	mutable bool bDirty;
};
