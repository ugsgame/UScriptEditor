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

#include "CodeEditorStyle.h"
#include "CodeProjectItem.h"
#include "CodeEditorUtils.h"
#include "CPPRichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "SCodeEditableText.h"

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
		.BorderImage(FCodeEditorStyle::Get().GetBrush("TextEditor.Border"))
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
			SAssignNew(OverlayWidget, SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SNew(SBox)
					.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
					.MinDesiredWidth(500.f).MinDesiredHeight(300.f)
					[
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
									//.BorderImage(FEditorStyle::GetBrush("Graph.Node.Body"))
									[
										SAssignNew(LineCounter, SListView<TSharedPtr<FString>>)
										.OnSelectionChanged(this, &SCodeEditor::OnSelectedLineCounterItem)
										.OnMouseButtonDoubleClick(this, &SCodeEditor::OnDoubleClick)
										.OnGenerateRow(this, &SCodeEditor::OnGenerateLineCounter)
										.ScrollbarVisibility(EVisibility::Collapsed)
										.ListItemsSource(&LineCount).ItemHeight(14)
										.SelectionMode(ESelectionMode::Single)
									]
								]
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
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
					]
				]
			]
		];

	//TAttribute<FLinearColor> ColorAndOpacity(FLinearColor(0, 0, 0, 0));
	//LineCounter->SetForegroundColor(FSlateColor::UseForeground());

	SetLineCountList(GetLineCount());
}

void SCodeEditor::OnTextChanged(const FText& NewText)
{
	bDirty = true;

	SetLineCountList(GetLineCount());
	//Sync to the ScriptAsset?
	/*
	if (CodeProjectItem->ScriptDataAsset)
	{
		CodeProjectItem->ScriptDataAsset->CodeText = CodeEditableText->GetText().ToString();
		//Set Asset To Dirty
		CodeProjectItem->ScriptDataAsset->MarkPackageDirty();
	}
	*/
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
				CodeEditorUtils::SaveScriptAsset(CodeProjectItem->ScriptDataAsset);
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
	CodeEditableText->GoToLineColumn(LineNumber, ColumnNumber);
}

void SCodeEditor::SetLineCountList(const int32 Count) 
{
	LineCount.Empty();
	//
	for (int32 I = 1; I <= Count; I++) {
		FString ID = FString::Printf(TEXT("%i"), I);
		LineCount.Add(MakeShareable(new FString(*ID)));
	}///
	//
	if (LineCounter.IsValid()) { LineCounter->RequestListRefresh(); }
}

void SCodeEditor::OnSelectedLineCounterItem(TSharedPtr<FString>Item, ESelectInfo::Type SelectInfo)
{
	if (!Item.IsValid()) { return; }
	//
	const int32 LineID = FCString::Atoi(**Item.Get());
	//
	FSlateApplication::Get().SetKeyboardFocus(CodeEditableText.ToSharedRef());
	CodeEditableText->GoToLineColumn(LineID - 1, 0);
	CodeEditableText->SelectLine();
	//
	LineCounter->SetItemSelection(Item, false);
}

void SCodeEditor::OnDoubleClick(TSharedPtr<FString>Item)
{
	if (!Item.IsValid()) { return; }
	//
	const int32 LineID = FCString::Atoi(**Item.Get());

	//TODO:Add or remove breakpoint
	LineCounter->SetItemHighlighted(Item, !LineCounter->Private_IsItemHighlighted(Item));

	TSharedPtr<ITableRow> TableRow = LineCounter->WidgetFromItem(Item);
	//
}


TSharedRef<ITableRow> SCodeEditor::OnGenerateLineCounter(TSharedPtr<FString>Item, const TSharedRef<STableViewBase>&OwnerTable) {
	return
	SNew(SComboRow<TSharedRef<FString>>, OwnerTable)
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("Graph.Node.Body"))
		.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f))
		.Padding(FMargin(5.f, 0.f, 5.f, 0.f))
		[
			SNew(STextBlock)
			.Text(FText::FromString(*Item.Get()))
			.ColorAndOpacity(FSlateColor(FLinearColor(FColor(75, 185, 245, 225))))
			.Font(FCodeEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText").Font)
		]
	];//
}

#undef LOCTEXT_NAMESPACE
