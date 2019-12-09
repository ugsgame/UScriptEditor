// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VarWatcher.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"


#define LOCTEXT_NAMESPACE "FVarWatcher"

SVarWatcher* SVarWatcher::Ptr = nullptr;

void SVarWatcher::Construct(const FArguments& InArgs)
{
	StartupModule();

	OnSpawnPluginTab(FSpawnTabArgs(TSharedPtr<SWindow>(), FTabId()));
}

TSharedRef<ITableRow> SVarWatcher::HandleVarTreeGenerateRow(TSharedRef<FVarNode> InVarNode, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SVarTreeWidgetItem, OwnerTable)
		.VarInfoToVisualize(InVarNode)
		;
}


void SVarWatcher::HandleVarTreeGetChildren(TSharedRef<FVarNode> InVarNode, TArray<TSharedRef<FVarNode>>& OutChildren)
{
	InVarNode->GetChild(OutChildren);
}


void SVarWatcher::HandleVarTreeSelectionChanged(TSharedPtr<FVarNode>, ESelectInfo::Type /*SelectInfo*/)
{
	bNeedUpdateData = true;
}


void SVarWatcher::HandleNodeExpansion(TSharedRef<FVarNode> VarNode, bool bIsExpaned)
{
	VarNode->bNeedExpandNextUpdate = bIsExpaned;
	bNeedUpdateData = true;
}

void SVarWatcher::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	Ptr = this;


	auto TickDelegate = FTickerDelegate::CreateLambda(
		[&](float DeltaTime)->bool {this->Update(DeltaTime); return true; }
	);
	FTicker::GetCoreTicker().AddTicker(TickDelegate, 0);
}

void SVarWatcher::ShutdownModule()
{
}

void SVarWatcher::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	bNeedTickTreeView = true;
	bShowFunction = false;

	TSharedPtr<SOverlay>OverlayWidget; this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked_Lambda([&]() {return bNeedTickTreeView ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([&](const ECheckBoxState NewState) { bNeedTickTreeView = (NewState == ECheckBoxState::Checked) ? true : false; })
					[
						SNew(SBox)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Padding(FMargin(4.0, 2.0))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("VarWatcherNeedTick", "NeedTick"))
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SCheckBox)
					.Style(FCoreStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked_Lambda([&]() {return bShowFunction ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([&](const ECheckBoxState NewState) { bShowFunction = (NewState == ECheckBoxState::Checked) ? true : false; })
					[
						SNew(SBox)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						.Padding(FMargin(4.0, 2.0))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("VarWatcherShowFunction", "ShowFunction"))
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.Padding(0)
				.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					SAssignNew(VarTreePtr, SVarsTree)
					.ItemHeight(24.0f)
					.TreeItemsSource(&VarTreeRoot)
					.OnGenerateRow_Raw(this, &SVarWatcher::HandleVarTreeGenerateRow)
					.OnGetChildren_Raw(this, &SVarWatcher::HandleVarTreeGetChildren)
					.OnSelectionChanged_Raw(this, &SVarWatcher::HandleVarTreeSelectionChanged)
					.OnExpansionChanged_Raw(this, &SVarWatcher::HandleNodeExpansion)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+SHeaderRow::Column(SVarTreeWidgetItem::Col_Name)
						.DefaultLabel(LOCTEXT("VarName1", "Key"))
						.FillWidth(20.0f)

						+ SHeaderRow::Column(SVarTreeWidgetItem::Col_Value)
						.DefaultLabel(LOCTEXT("VarValue1", "Value"))
						.FillWidth(20.0f)
					)
				]
			]
		];
}


void SVarWatcher::Update(float Delta)
{
	if (bNeedTickTreeView)
		bNeedUpdateData = true;
}		


#undef LOCTEXT_NAMESPACE
	

FName SVarTreeWidgetItem::Col_Name = "VarName";
FName SVarTreeWidgetItem::Col_Value = "VarValue";

TSharedRef<SWidget> SVarTreeWidgetItem::GenerateWidgetForColumn(const FName& ColumnName)
{
	if (ColumnName == Col_Name)
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text_Lambda([&]()->FText {return VarInfoNode->GetName(); })
			];
	}
	else
	{
		return SNew(STextBlock)
			.Text_Lambda([&]()->FText {return VarInfoNode->GetValue(); })
			;
	}
}
