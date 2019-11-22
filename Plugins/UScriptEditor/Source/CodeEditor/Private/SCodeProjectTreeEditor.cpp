// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCodeProjectTreeEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SHeader.h"
#include "CodeEditorStyle.h"
#include "CodeProject.h"
#include "CodeProjectEditor.h"
#include "CodeEditorUtils.h"
#include "SProjectViewItem.h"
#include "DirectoryScanner.h"
#include "Widgets/Images/SThrobber.h"


#define LOCTEXT_NAMESPACE "CodeProjectTreeEditor"

TWeakPtr<SCodeProjectTreeEditor> SCodeProjectTreeEditor::CodeProjectTreeEditor;

void SCodeProjectTreeEditor::Construct(const FArguments& InArgs, UCodeProjectItem* InCodeProject, UCodeProjectItem* InScriptProject)
{
	check(InCodeProject);
	check(InScriptProject);
	CodeProject = InCodeProject;
	ScriptProject = InScriptProject;

	EditingProject = InScriptProject;

	TSharedPtr<SCodeProjectTreeEditor> ThisPtr(SharedThis(this));
	CodeProjectTreeEditor = ThisPtr;


	if (!ProjectTree.IsValid())
	{
		SAssignNew(ProjectTree, STreeView<UCodeProjectItem*>)
			.TreeItemsSource(&EditingProject->Children)
			.OnGenerateRow(this, &SCodeProjectTreeEditor::OnGenerateRow)
			.OnGetChildren(this, &SCodeProjectTreeEditor::OnGetChildren)
			.OnMouseButtonDoubleClick(this, &SCodeProjectTreeEditor::HandleMouseButtonDoubleClick)
			.HighlightParentNodesForSelection(true);
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCodeEditorStyle::Get().GetBrush("ProjectEditor.Border"))
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
						.OnClicked(this, &SCodeProjectTreeEditor::OnClickedScriptProject)
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.FillWidth(2)
					[
						SNew(SButton)
						.Text(LOCTEXT("ButtonCodeProject", "SourceCode"))
						.OnClicked(this,&SCodeProjectTreeEditor::OnClickedCodeProject)
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
				.Visibility(this, &SCodeProjectTreeEditor::GetThrobberVisibility)
			]
		]
	];

	//
	
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &SCodeProjectTreeEditor::RescanAllFiles);
	}
	else
	{
		//TODO:Reflash
		//CodeProject->Children.Empty();
		//ScriptProject->Children.Empty();

		CodeProject->RescanChildren();
		ScriptProject->RescanChildren();
		//
	}
	
}

void SCodeProjectTreeEditor::ExpanedScriptItem(UCodeProjectItem* Item,bool ShouldExpandItem, bool Always)
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

void SCodeProjectTreeEditor::ExpanedAllScriptItems()
{
	ProjectTree->SetTreeItemsSource(&ScriptProject->Children);
	EditingProject = ScriptProject;

	ExpanedItemChildren(EditingProject);
}

void SCodeProjectTreeEditor::RescanScripts()
{
	ScriptProject->Children.Empty();
	ScriptProject->RescanChildren();
}

void SCodeProjectTreeEditor::RescanCodes()
{
	CodeProject->Children.Empty();
	CodeProject->RescanChildren();
}

void SCodeProjectTreeEditor::RescanAllFiles()
{
	RescanScripts();
	RescanCodes();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnFilesLoaded().RemoveAll(this);
}

void SCodeProjectTreeEditor::RequestRefresh()
{
	ProjectTree->RequestTreeRefresh();
}

void SCodeProjectTreeEditor::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if(FDirectoryScanner::Tick())
	{
		ProjectTree->SetTreeItemsSource(&EditingProject->Children);
	}

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

FName SCodeProjectTreeEditor::GetIconForItem(UCodeProjectItem* Item) const
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

TSharedRef<class ITableRow> SCodeProjectTreeEditor::OnGenerateRow(UCodeProjectItem* Item, const TSharedRef<STableViewBase >& OwnerTable)
{
	return
	SNew(STableRow<UCodeProjectItem*>, OwnerTable)
	[
		SNew(SProjectViewItem)
		.Text(FText::FromString(Item->Name))
		.IconName(GetIconForItem(Item))
	];
}

void SCodeProjectTreeEditor::OnGetChildren(UCodeProjectItem* Item, TArray<UCodeProjectItem*>& OutChildItems)
{
	OutChildItems = Item->Children;
}

EVisibility SCodeProjectTreeEditor::GetThrobberVisibility() const
{ 
	return FDirectoryScanner::IsScanning() ? EVisibility::Visible : EVisibility::Hidden; 
}

void SCodeProjectTreeEditor::HandleMouseButtonDoubleClick(UCodeProjectItem* Item) const
{
	if(Item->Type == ECodeProjectItemType::File)
	{
		FCodeProjectEditor::Get()->OpenFileForEditing(Item);
	}
}

void SCodeProjectTreeEditor::ExpanedItem(UCodeProjectItem* Item, bool ShouldExpandItem) const
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

void SCodeProjectTreeEditor::ExpanedItemChildren(UCodeProjectItem* Item) const
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

FReply SCodeProjectTreeEditor::OnClickedCodeProject()
{
	ProjectTree->SetTreeItemsSource(&CodeProject->Children);
	EditingProject = CodeProject;

	return FReply::Handled();
}

FReply SCodeProjectTreeEditor::OnClickedScriptProject()
{
	ProjectTree->SetTreeItemsSource(&ScriptProject->Children);
	EditingProject = ScriptProject;

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
