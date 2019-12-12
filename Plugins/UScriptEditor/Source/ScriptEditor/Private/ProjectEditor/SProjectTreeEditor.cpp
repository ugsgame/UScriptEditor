// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SProjectTreeEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SHeader.h"
#include "ScriptEditorStyle.h"
#include "SourceProject.h"
#include "ScriptEditor.h"
#include "ScriptEditorUtils.h"
#include "ScriptEditorSetting.h"
#include "SProjectViewItem.h"
#include "DirectoryScanner.h"
#include "Widgets/Images/SThrobber.h"

#include "ModuleManager.h"
#include "AssetRegistryModule.h"


#define LOCTEXT_NAMESPACE "ProjectTreeEditor"

TWeakPtr<SProjectTreeEditor> SProjectTreeEditor::ProjectTreeEditor;

void SProjectTreeEditor::Construct(const FArguments& InArgs, UCodeProjectItem* InSourceProject, UCodeProjectItem* InScriptProject)
{
	check(InSourceProject);
	check(InScriptProject);
	SourceProject = InSourceProject;
	ScriptProject = InScriptProject;

	EditingProject = InScriptProject;

	InSourceProject->OnDirectoryScannedOver.BindRaw(this, &SProjectTreeEditor::OnRescanOver);
	ScriptProject->OnDirectoryScannedOver.BindRaw(this, &SProjectTreeEditor::OnRescanOver);

	TSharedPtr<SProjectTreeEditor> ThisPtr(SharedThis(this));
	ProjectTreeEditor = ThisPtr;


	if (!ProjectTree.IsValid())
	{
		SAssignNew(ProjectTree, STreeView<UCodeProjectItem*>)
			.TreeItemsSource(&EditingProject->Children)
			.OnGenerateRow(this, &SProjectTreeEditor::OnGenerateRow)
			.OnGetChildren(this, &SProjectTreeEditor::OnGetChildren)
			.OnMouseButtonDoubleClick(this, &SProjectTreeEditor::HandleMouseButtonDoubleClick)
			.HighlightParentNodesForSelection(true);
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FScriptEditorStyle::Get().GetBrush("ProjectEditor.Border"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(2)
				[
					SAssignNew(ScriptProjectButton,SButton)
					//.ButtonStyle(FCoreStyle::Get(), "ToggleButtonCheckbox")
					.Text(LOCTEXT("ButtonScriptProject", "ScriptContent"))
					.OnClicked(this, &SProjectTreeEditor::OnClickedScriptProject)
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(2)
				[
					SAssignNew(SourceProjectButton,SButton)
					//.ButtonStyle(FCoreStyle::Get(), "ToggleButtonCheckbox")
					.Text(LOCTEXT("ButtonCodeProject", "SourceCode"))
					.OnClicked(this, &SProjectTreeEditor::OnClickedCodeProject)
				]
 			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.Padding(0)
				[
					ProjectTree.ToSharedRef()
				]
			]
		]
	];

	ScriptProjectButton->SetBorderBackgroundColor(SelectedColor);
	SourceProjectButton->SetBorderBackgroundColor(UnSelectedColor);

	//
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &SProjectTreeEditor::RescanAllFiles);
	}
	else
	{
		//TODO:Reflash
		SourceProject->Children.Empty();
		ScriptProject->Children.Empty();

		SourceProject->RescanChildren();
		ScriptProject->RescanChildren();
		//
	}
	
}

void SProjectTreeEditor::ExpanedScriptItem(UCodeProjectItem* Item,bool ShouldExpandItem, bool Always)
{
	if (Item)
	{
		ProjectTree->SetTreeItemsSource(&ScriptProject->Children);
		EditingProject = ScriptProject;

		ProjectTree->SetSelection(Item, ESelectInfo::OnMouseClick);

		if (Always)
			ExpanedItem(Item, ShouldExpandItem);
		else
			ProjectTree->SetItemExpansion(Item, ShouldExpandItem);
	}
}

void SProjectTreeEditor::ExpanedAllScriptItems()
{
	ProjectTree->SetTreeItemsSource(&ScriptProject->Children);
	EditingProject = ScriptProject;

	ExpanedAllEditingItems();
}

void SProjectTreeEditor::ExpanedEditingItem(class UCodeProjectItem* Item, bool ShouldExpandItem /*= true*/, bool Always /*= true*/)
{
	ProjectTree->SetSelection(Item, ESelectInfo::OnMouseClick);

	if (Always)
		ExpanedItem(Item, ShouldExpandItem);
	else
		ProjectTree->SetItemExpansion(Item, ShouldExpandItem);
}

void SProjectTreeEditor::ExpanedAllEditingItems()
{
	if (EditingProject)
		ExpanedItemChildren(EditingProject);
}

void SProjectTreeEditor::RescanScripts()
{
	ScriptProject->Children.Empty();
	ScriptProject->RescanChildren();
}

void SProjectTreeEditor::RescanSources()
{
	SourceProject->Children.Empty();
	SourceProject->RescanChildren();
}

void SProjectTreeEditor::RescanAllFiles()
{
	RescanScripts();
	RescanSources();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnFilesLoaded().RemoveAll(this);
}

void SProjectTreeEditor::RequestRefresh()
{
	ProjectTree->RequestTreeRefresh();
}

void SProjectTreeEditor::SwitchToScriptProject()
{
	ProjectTree->SetTreeItemsSource(&ScriptProject->Children);
	EditingProject = ScriptProject;
	ScriptProjectButton->SetBorderBackgroundColor(SelectedColor);
	SourceProjectButton->SetBorderBackgroundColor(UnSelectedColor);

	//TODO:Expanded editting items
}

void SProjectTreeEditor::SwitchToSourceProject()
{
	ProjectTree->SetTreeItemsSource(&SourceProject->Children);
	EditingProject = SourceProject;
	SourceProjectButton->SetBorderBackgroundColor(SelectedColor);
	ScriptProjectButton->SetBorderBackgroundColor(UnSelectedColor);

	//TODO:Expanded editting items
}

void SProjectTreeEditor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if(FDirectoryScanner::Tick())
	{
		ProjectTree->SetTreeItemsSource(&EditingProject->Children);
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FName SProjectTreeEditor::GetIconForItem(UCodeProjectItem* Item) const
{
	return Item->GetBrush();
}

TSharedRef<class ITableRow> SProjectTreeEditor::OnGenerateRow(UCodeProjectItem* Item, const TSharedRef<STableViewBase >& OwnerTable)
{
	return
	SNew(STableRow<UCodeProjectItem*>, OwnerTable)
	[
		SNew(SProjectViewItem)
		.Text(FText::FromString(Item->Name))
		.IconName(GetIconForItem(Item))
	];
}

void SProjectTreeEditor::OnGetChildren(UCodeProjectItem* Item, TArray<UCodeProjectItem*>& OutChildItems)
{
	OutChildItems = Item->Children;
}

EVisibility SProjectTreeEditor::GetThrobberVisibility() const
{ 
	return FDirectoryScanner::IsScanning() ? EVisibility::Visible : EVisibility::Hidden; 
}

void SProjectTreeEditor::HandleMouseButtonDoubleClick(UCodeProjectItem* Item) const
{
	if(Item->Type == ECodeProjectItemType::File)
	{
		FScriptEditor::Get()->OpenFileForEditing(Item);
	}
}

void SProjectTreeEditor::OnRescanOver()
{
	//Sort the items from EdittingFiles;
	TArray<UCodeProjectItem*> SortItems;
	for (FString FilePath:UScriptEdtiorSetting::Get()->EdittingFiles)
	{
		for (UCodeProjectItem* Item : UScriptEdtiorSetting::Get()->PreEdittingItems)
		{
			if (FilePath == Item->Path)
				SortItems.Add(Item);
		}
	}
	UScriptEdtiorSetting::Get()->EdittingFiles.Empty();
	FScriptEditor::Get()->CloseAllEditingFiles();
	//
	//Open PreEditting file tabs
	for (UCodeProjectItem* Item: SortItems)
	{
		FScriptEditor::Get()->OpenFileForEditing(Item);
		ExpanedEditingItem(Item);
	}
	UScriptEdtiorSetting::Get()->PreEdittingItems.Empty();
	SortItems.Empty();
}

void SProjectTreeEditor::ExpanedItem(UCodeProjectItem* Item, bool ShouldExpandItem) const
{
	if (Item)
	{
		ProjectTree->SetItemExpansion(Item, ShouldExpandItem);
		if (Item->Parent)
		{
			ExpanedItem(Item->Parent, ShouldExpandItem);
		}
	}
}

void SProjectTreeEditor::ExpanedItemChildren(UCodeProjectItem* Item) const
{
	if (Item)
	{
		ProjectTree->SetItemExpansion(Item, true);
		if (Item->Children.Num()>0)
		{
			for (UCodeProjectItem* Child: Item->Children)
			{
				ExpanedItemChildren(Child);
			}
		}
	}
}

FReply SProjectTreeEditor::OnClickedCodeProject()
{
	SwitchToSourceProject();
	return FReply::Handled();
}

FReply SProjectTreeEditor::OnClickedScriptProject()
{
	SwitchToScriptProject();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
