// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "SProjectTreeEditor.h"
#include "Widgets/Views/STreeView.h"

class SProjectTreeEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SProjectTreeEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UCodeProjectItem* InSourceProject, class UCodeProjectItem* InScriptProject);

	static TSharedPtr<SProjectTreeEditor> Get()
	{
		return ProjectTreeEditor.Pin();
	}

	void ExpanedScriptItem(class UCodeProjectItem* Item, bool ShouldExpandItem = true, bool Always = true);

	void ExpanedAllScriptItems();

	void RescanScripts();
	void RescanCodes();

	void RescanAllFiles();

	void RequestRefresh();
private:
	/** Begin SWidget interface */
	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	/** End SWidget interface */

	TSharedRef<class ITableRow> OnGenerateRow(class UCodeProjectItem* Item, const TSharedRef<class STableViewBase>& TableView);

	void OnGetChildren(class UCodeProjectItem* Item, TArray<class UCodeProjectItem*>& OutChildItems);

	EVisibility GetThrobberVisibility() const;

	FName GetIconForItem(class UCodeProjectItem* Item) const;

	void ExpanedItem(class UCodeProjectItem* Item,bool ShouldExpandItem = true) const;

	void ExpanedItemChildren(class UCodeProjectItem* Item) const;

	void HandleMouseButtonDoubleClick(class UCodeProjectItem* Item) const;

	virtual FReply OnClickedCodeProject();
	virtual FReply OnClickedScriptProject();

private:
	class  UCodeProjectItem* SourceProject;
	class  UCodeProjectItem* ScriptProject;

	class UCodeProjectItem* EditingProject;

	TSharedPtr<STreeView<class UCodeProjectItem*>> ProjectTree;

	static TWeakPtr<SProjectTreeEditor> ProjectTreeEditor;

};
