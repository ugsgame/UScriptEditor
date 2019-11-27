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
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.0f, 5.0f))
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("ButtonScriptProject", "ScriptContent"))
						.OnClicked(this, &SProjectTreeEditor::OnClickedScriptProject)
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("ButtonCodeProject", "SourceCode"))
						.OnClicked(this,&SProjectTreeEditor::OnClickedCodeProject)
					]
				]
	 			+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(1)
	 			[
					ProjectTree.ToSharedRef()
	 			]
 			]
			+SOverlay::Slot()
			.VAlign(VAlign_Bottom)
			.Padding(10.0f)
			[
				SNew(SThrobber)
				.Visibility(this, &SProjectTreeEditor::GetThrobberVisibility)
			]
		]
	];

	//
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &SProjectTreeEditor::RescanAllFiles);
	}
	else
	{
		//TODO:Reflash
		//CodeProject->Children.Empty();
		//ScriptProject->Children.Empty();

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

	ExpanedItemChildren(EditingProject);
}

void SProjectTreeEditor::RescanScripts()
{
	ScriptProject->Children.Empty();
	ScriptProject->RescanChildren();
}

void SProjectTreeEditor::RescanCodes()
{
	SourceProject->Children.Empty();
	SourceProject->RescanChildren();
}

void SProjectTreeEditor::RescanAllFiles()
{
	RescanScripts();
	RescanCodes();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnFilesLoaded().RemoveAll(this);
}

void SProjectTreeEditor::RequestRefresh()
{
	ProjectTree->RequestTreeRefresh();
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
	switch(Item->Type)
	{
	case ECodeProjectItemType::Project:
		return "ProjectEditor.Icon.Project";
	case ECodeProjectItemType::Folder:
		return "ProjectEditor.Icon.Folder";
	case ECodeProjectItemType::File:
		return "ProjectEditor.Icon.File";
	default:
		return "ProjectEditor.Icon.GenericFile";
	}
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
	ProjectTree->SetTreeItemsSource(&SourceProject->Children);
	EditingProject = SourceProject;

	return FReply::Handled();
}

FReply SProjectTreeEditor::OnClickedScriptProject()
{
	ProjectTree->SetTreeItemsSource(&ScriptProject->Children);
	EditingProject = ScriptProject;

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
