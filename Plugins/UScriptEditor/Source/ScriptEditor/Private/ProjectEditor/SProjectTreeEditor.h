﻿// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

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

	void ExpanedEditingItem(class UCodeProjectItem* Item, bool ShouldExpandItem = true, bool Always = true);
	void ExpanedAllEditingItems();
;
	void RescanScripts();
	void RescanSources();

	void RescanAllFiles();

	void RequestRefresh();

	void SwitchToScriptProject();
	void SwitchToSourceProject();
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

	void OnExpansionChanged(class UCodeProjectItem* Item,bool InChagned);

	void OnRescanOver();

	virtual FReply OnClickedCodeProject();
	virtual FReply OnClickedScriptProject();
public:

	TArray<class UCodeProjectItem*> ExpandedItems;

private:
	class  UCodeProjectItem* SourceProject;
	class  UCodeProjectItem* ScriptProject;

	class UCodeProjectItem* EditingProject;

	TSharedPtr<STreeView<class UCodeProjectItem*>> ProjectTree;

	TSharedPtr<SButton> ScriptProjectButton;
	TSharedPtr<SButton> SourceProjectButton;

	FColor SelectedColor  = FColor(244, 145, 65);
	FColor UnSelectedColor = FColor(200, 200, 200);

	static TWeakPtr<SProjectTreeEditor> ProjectTreeEditor;

};
