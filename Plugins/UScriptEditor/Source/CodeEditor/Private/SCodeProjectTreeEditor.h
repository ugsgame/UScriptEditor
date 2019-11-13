// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "CodeProjectItem.h"
#include "Widgets/Views/STreeView.h"

class SCodeProjectTreeEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCodeProjectTreeEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UCodeProjectItem* InCodeProject, class UCodeProjectItem* InScriptProject);

private:
	/** Begin SWidget interface */
	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	/** End SWidget interface */

	TSharedRef<class ITableRow> OnGenerateRow(class UCodeProjectItem* Item, const TSharedRef<class STableViewBase>& TableView);

	void OnGetChildren(class UCodeProjectItem* Item, TArray<class UCodeProjectItem*>& OutChildItems);

	EVisibility GetThrobberVisibility() const;

	FName GetIconForItem(class UCodeProjectItem* Item) const;

	void HandleMouseButtonDoubleClick(class UCodeProjectItem* Item) const;

private:
	class  UCodeProjectItem* CodeProject;
	class  UCodeProjectItem* ScriptProject;

	TSharedPtr<STreeView<class UCodeProjectItem*>> CodeProjectTree;
	TSharedPtr<STreeView<class UCodeProjectItem*>> ScriptProjectTree;
};
