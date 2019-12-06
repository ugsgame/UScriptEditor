// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCodeEditor.h"
#include "Misc/FileHelper.h"
#include "Framework/Text/TextLayout.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "ScriptEditorStyle.h"
#include "CodeProjectItem.h"
#include "ScriptEditorUtils.h"
#include "CPPRichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "SCodeEditableText.h"

#include "ScriptEditor.h"
#include "SScriptDebugger.h"

#include "Editor/EditorStyle/Public/EditorStyle.h"

#define LOCTEXT_NAMESPACE "CodeEditor"


void SCodeEditor::Construct(const FArguments& InArgs, UCodeProjectItem* InCodeProjectItem)
{
	bDirty = false;

	check(InCodeProjectItem);
	CodeProjectItem = InCodeProjectItem;

	FString FileText = "File Loading, please wait";
	FFileHelper::LoadFileToString(FileText, *InCodeProjectItem->Path);


	TSharedRef<FCPPRichTextSyntaxHighlighterTextLayoutMarshaller> RichTextMarshaller = FCPPRichTextSyntaxHighlighterTextLayoutMarshaller::Create(
			FCPPRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle()
			);

	HorizontalScrollbar = 
		SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(14.0f, 14.0f));

	VerticalScrollbar = 
		SNew(SScrollBar)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(14.0f, 14.0f));

	/*
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SGridPanel)
			.FillColumn(0, 1.0f)
			.FillRow(0, 1.0f)
			+SGridPanel::Slot(0, 0)
			[
 				SAssignNew(CodeEditableText, SCodeEditableText)
 				.OnTextChanged(this, &SCodeEditor::OnTextChanged)
 				.OnTextCommitted(this, &SCodeEditor::OnTextCommitted)
 				.Text(FText::FromString(FileText))
 				.Marshaller(RichTextMarshaller)
 				.HScrollBar(HorizontalScrollbar)
 				.VScrollBar(VerticalScrollbar)
			]
			+SGridPanel::Slot(1, 0)
			[
				VerticalScrollbar.ToSharedRef()
			]
			+SGridPanel::Slot(0, 1)
			[
				HorizontalScrollbar.ToSharedRef()
			]
		]
	];
	*/
	
	
	TSharedPtr<SOverlay>OverlayWidget; this->ChildSlot
		[
// 			SAssignNew(OverlayWidget, SOverlay)
// 			+ SOverlay::Slot()
// 			[
// 				SNew(SVerticalBox)
// 				+ SVerticalBox::Slot()
// 				[
// 					SNew(SBox)
// 					.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
// 					.MinDesiredWidth(500.f).MinDesiredHeight(300.f)
// 					[
						SNew(SBorder)
						.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
						.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
						[
							SAssignNew(VS_SCROLL_BOX, SScrollBox)
							.OnUserScrolled(this, &SCodeEditor::OnVerticalScroll)
							.Orientation(EOrientation::Orient_Vertical)
							.ScrollBarThickness(FVector2D(8.f, 8.f))
							+ SScrollBox::Slot()
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Fill).HAlign(HAlign_Left).AutoWidth()
								[
									SNew(SBorder)
									.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
									//.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
									.BorderImage(FEditorStyle::GetBrush("NoBorder"))
									[
										SAssignNew(LineCounter, SListView<FCodeLineNode_Ptr>)
										.OnSelectionChanged(this, &SCodeEditor::OnSelectedLineCounterItem)
										.OnMouseButtonDoubleClick(this, &SCodeEditor::OnDoubleClickLineCounterItem)
										.OnGenerateRow(this, &SCodeEditor::OnGenerateLineCounter)
										.ScrollbarVisibility(EVisibility::Collapsed)
										.ListItemsSource(&LineCount).ItemHeight(14)
										.SelectionMode(ESelectionMode::Single)
									]
								]
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Fill).HAlign(HAlign_Fill).AutoWidth()
								[
									SAssignNew(CodeEditableText, SCodeEditableText)
									.OnTextChanged(this, &SCodeEditor::OnTextChanged)
									.OnTextCommitted(this, &SCodeEditor::OnTextCommitted)
									//.IsEnabled(this,&SMGC_CodeEditorCore::IsScriptEditable)
									.Text(FText::FromString(FileText))
									.VScrollBar(VerticalScrollbar)
									.HScrollBar(HorizontalScrollbar)
									.Marshaller(RichTextMarshaller)
									.CanKeyboardFocus(true)
									.IsReadOnly(false)
								]
							]
						]
// 					]
// 				]
// 			]
		];

	//Add Line Number
	SetLineCountList(GetLineCount());
}

void SCodeEditor::OnTextChanged(const FText& NewText)
{
	bDirty = true;

	SetLineCountList(GetLineCount());

	//Sync to the ScriptAsset?
	
	if (CodeProjectItem->ScriptDataAsset)
	{
		CodeProjectItem->ScriptDataAsset->CodeText = CodeEditableText->GetText().ToString();
		//Set Asset To Dirty
		CodeProjectItem->ScriptDataAsset->MarkPackageDirty();
	}
	
	//
}

void SCodeEditor::OnTextCommitted(const FText &NewText, ETextCommit::Type ComtInfo)
{
	SetLineCountList(GetLineCount());
}

void SCodeEditor::OnVerticalScroll(float Offset) 
{
	VerticalScrollbar->SetState(VS_SCROLL_BOX->GetScrollOffset(), VS_SCROLL_BOX->GetViewOffsetFraction());
}

bool SCodeEditor::Save() const
{
	if(bDirty)
	{
		bool bResult = FFileHelper::SaveStringToFile(CodeEditableText->GetText().ToString(), *CodeProjectItem->Path);
		if(bResult)
		{
			bDirty = false;
			//Save to asset
			if (CodeProjectItem->ScriptDataAsset)
			{
				CodeProjectItem->ScriptDataAsset->CodeText = CodeEditableText->GetText().ToString();
				ScriptEditorUtils::SaveScriptAsset(CodeProjectItem->ScriptDataAsset);
			}
		}

		return bResult;
	}
	return true;
}

bool SCodeEditor::CanSave() const
{
	return bDirty;
}

void SCodeEditor::Browser() const
{
	if (CodeProjectItem->ScriptDataAsset)
	{
		ScriptEditorUtils::BrowserToScriptAsset(CodeProjectItem->ScriptDataAsset);
	}
}

int32 SCodeEditor::GetLineCount() const
{
	TArray<FString>Array;
	int32 Count = 0;
	//
	FString Text = CodeEditableText->GetText().ToString();
	Count = Text.ParseIntoArray(Array, TEXT("\n"), false);
	//
	return Count;
}

void SCodeEditor::GotoLineAndColumn(int32 LineNumber, int32 ColumnNumber)
{
	FSlateApplication::Get().SetKeyboardFocus(CodeEditableText.ToSharedRef());
	CodeEditableText->GoToLineColumn(LineNumber, ColumnNumber);
	CodeEditableText->SelectLine();

	VS_SCROLL_BOX->SetScrollOffset((VS_SCROLL_BOX->GetScrollOffsetOfEnd()/GetLineCount ())* LineNumber);
}

FText SCodeEditor::GetLineAndColumn() const
{
	int32 Line;
	int32 Column;
	CodeEditableText->GetLineAndColumn(Line, Column);

	FString LineAndColumn = FString::Printf(TEXT("Line: %d Column: %d"), Line + 1, Column);

	return FText::FromString(LineAndColumn);
}

void SCodeEditor::SetLineCountList(const int32 Count) 
{
	LineCount.Empty();
	//
	for (int32 I = 1; I <= Count; I++) {
		LineCount.Add(MakeShareable(new FCodeLineNode(I, CodeProjectItem->Path)));
	}///
	//
	if (LineCounter.IsValid()) { LineCounter->RequestListRefresh(); }
}

void SCodeEditor::OnSelectedLineCounterItem(FCodeLineNode_Ptr Item, ESelectInfo::Type SelectInfo)
{
	if (!Item.IsValid()) { return; }
	//
	const int32 LineID = Item->Line;
	//
	FSlateApplication::Get().SetKeyboardFocus(CodeEditableText.ToSharedRef());
	CodeEditableText->GoToLineColumn(LineID - 1, 0);
	CodeEditableText->SelectLine();
	//
	LineCounter->SetItemSelection(Item, false);
}

void SCodeEditor::OnDoubleClickLineCounterItem(FCodeLineNode_Ptr Item)
{
	if (!Item.IsValid()) { return; }
	//
	const int32 LineID = Item->Line;

	//TODO:Add or remove breakpoint
	//LineCounter->SetItemHighlighted(Item, !LineCounter->Private_IsItemHighlighted(Item));

	TSharedPtr<ITableRow> TableRow = LineCounter->WidgetFromItem(Item);
	//
}

TSharedRef<ITableRow> SCodeEditor::OnGenerateLineCounter(FCodeLineNode_Ptr Item, const TSharedRef<STableViewBase>&OwnerTable) 
{
	Item->FilePath = CodeProjectItem->Path;

	return SNew(SCodeLineItem, OwnerTable, Item);
}

#undef LOCTEXT_NAMESPACE

void SCodeLineItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FCodeLineNode_Ptr Line)
{
	CodeLine = Line;
 	CodeLine->HasBreakPoint = FScriptEditor::Get()->HasBreakPoint(CodeLine->FilePath, CodeLine->Line);

	STableRow<FCodeLineNode_Ptr>::Construct(STableRow<FCodeLineNode_Ptr>::FArguments(), InOwnerTableView);
	ChildSlot
		[
			SNew(SBorder)
			//.BorderImage(FEditorStyle::GetBrush("Graph.Node.Body"))
			.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f))
			.ForegroundColor(FSlateColor::UseForeground())
			.Padding(FMargin(5.f, 0.f, 5.f, 0.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				[
					SNew(SBox)
					[
						SAssignNew(BreakPointCheckBox, SCheckBox)
						.IsChecked(CodeLine->HasBreakPoint ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
						.OnCheckStateChanged(this, &SCodeLineItem::OnClickBreakPoint)
						.CheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
						.CheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
						.CheckedPressedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
						.UncheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Null"))
						.UncheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Null"))
					]
				]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::FormatAsNumber(CodeLine->Line)))
					.ColorAndOpacity(FSlateColor(FLinearColor(FColor(75, 185, 245, 225))))
					.Font(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText").Font)
				]
			]
		]
	];
}

void SCodeLineItem::OnClickBreakPoint(const ECheckBoxState NewCheckedState)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		CodeLine->HasBreakPoint = true;
	}
	else
	{
		CodeLine->HasBreakPoint = false;
	}

	FScriptEditor::Get()->ToggleBreakPoint(CodeLine->FilePath, CodeLine->Line);
	SScriptDebugger::Get()->SaveDebuggerConfig();
}

void SCodeLineItem::OnBreakConditionCommit(const FText& ConditionText, ETextCommit::Type CommitType)
{
	TSharedPtr<FScriptBreakPointNode> BreakPointNode = FScriptEditor::Get()->GetViewBreakPoint(CodeLine->FilePath, CodeLine->Line);
	if (BreakPointNode.IsValid())
	{
		BreakPointNode->HitCondition = ConditionText;
		UScriptDebuggerSetting::Get(false)->BreakConditionChange();
		UScriptDebuggerSetting::Get(true)->BreakConditionChange();
	}
}

