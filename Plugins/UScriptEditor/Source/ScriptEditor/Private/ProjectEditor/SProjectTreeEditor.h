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

	void Construct(const FArguments& InArgs, class UScriptProjectItem* InSourceProject, class UScriptProjectItem* InScriptProject);

	static TSharedPtr<SProjectTreeEditor> Get()
	{
		return ProjectTreeEditor.Pin();
	}

	void ExpanedScriptItem(class UScriptProjectItem* Item, bool ShouldExpandItem = true, bool Always = true);
	void ExpanedAllScriptItems();

	void ExpanedEditingItem(class UScriptProjectItem* Item, bool ShouldExpandItem = true, bool Always = true);
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

	TSharedRef<class ITableRow> OnGenerateRow(class UScriptProjectItem* Item, const TSharedRef<class STableViewBase>& TableView);

	void OnGetChildren(class UScriptProjectItem* Item, TArray<class UScriptProjectItem*>& OutChildItems);

	EVisibility GetThrobberVisibility() const;

	FName GetIconForItem(class UScriptProjectItem* Item) const;

	void ExpanedItem(class UScriptProjectItem* Item,bool ShouldExpandItem = true) const;

	void ExpanedItemChildren(class UScriptProjectItem* Item) const;

	void HandleMouseButtonDoubleClick(class UScriptProjectItem* Item) const;

	void OnExpansionChanged(class UScriptProjectItem* Item,bool InChagned);

	void OnRescanOver();

	virtual FReply OnClickedCodeProject();
	virtual FReply OnClickedScriptProject();
public:

	TArray<class UScriptProjectItem*> ExpandedItems;

private:
	class  UScriptProjectItem* SourceProject;
	class  UScriptProjectItem* ScriptProject;

	class UScriptProjectItem* EditingProject;

	TSharedPtr<STreeView<class UScriptProjectItem*>> ProjectTree;

	TSharedPtr<SButton> ScriptProjectButton;
	TSharedPtr<SButton> SourceProjectButton;

	FColor SelectedColor  = FColor(244, 145, 65);
	FColor UnSelectedColor = FColor(200, 200, 200);

	static TWeakPtr<SProjectTreeEditor> ProjectTreeEditor;

};
