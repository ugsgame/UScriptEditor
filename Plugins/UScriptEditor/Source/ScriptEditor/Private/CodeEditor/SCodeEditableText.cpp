// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCodeEditableText.h"
#include "ScriptEditorStyle.h"


void SCodeEditableText::Construct( const FArguments& InArgs )
{
	SMultiLineEditableText::Construct(
		SMultiLineEditableText::FArguments()
		.Font(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText").Font)
		.TextStyle(&FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText"))
		.Text(InArgs._Text)
		.Marshaller(InArgs._Marshaller)
		.AutoWrapText(false)
		.Margin(0.0f)
		.HScrollBar(InArgs._HScrollBar)
		.VScrollBar(InArgs._VScrollBar)
		.OnTextChanged(InArgs._OnTextChanged)
		.OnCursorMoved(this, &SCodeEditableText::OnCursorMoved)
	);
}

void SCodeEditableText::GoToLineColumn(int32 Line, int32 Column)
{
	FTextLocation Location(Line, Column);
	//
	GoTo(Location);
	ScrollTo(Location);

	GEditor->AddOnScreenDebugMessage(1, 5, FColor::Red, FString("Line:") + FString::FromInt(Line) + ":" + FString::FromInt(Column));


}


void SCodeEditableText::GetLineAndColumn(int32 & Line, int32 & Column)
{
	Line = CurrentLine;
	Column = CurrentColumn;
}

void SCodeEditableText::SelectLine() {
	EditableTextLayout->JumpTo(ETextLocation::EndOfLine, ECursorAction::MoveCursor);
	EditableTextLayout->JumpTo(ETextLocation::BeginningOfLine, ECursorAction::SelectText);
}

void SCodeEditableText::DeleteSelectedText() {
	EditableTextLayout->DeleteSelectedText();
}

FReply SCodeEditableText::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	FReply Reply = FReply::Unhandled();

	const TCHAR Character = InCharacterEvent.GetCharacter();
	if(Character == TEXT('\t'))
	{
		if (!IsTextReadOnly())
		{
			FString String;
			String.AppendChar(Character);
			InsertTextAtCursor(String);
			Reply = FReply::Handled();
		}
		else
		{
			Reply = FReply::Unhandled();
		}
	}
	else
	{
		Reply = SMultiLineEditableText::OnKeyChar( MyGeometry, InCharacterEvent );
	}

	return Reply;
}
