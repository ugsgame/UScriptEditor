// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCodeEditableText.h"
#include "ScriptEditorStyle.h"
#include "ScriptEditorCommands.h"
#include "UICommandList.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ScriptEditorUtils.h"
#include "MultiBoxBuilder.h"
#include "SScriptActionMenu.h"
#include "GraphEditor.h"
#include "SlateApplication.h"

#define  LOCTEXT_NAMESPACE "CodeEditableText"


void SCodeEditableText::Construct(const FArguments& InArgs)
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
		.ContextMenuExtender(this, &SCodeEditableText::ContextMenuExtender)
	);

	ExtenderCommands = MakeShareable(new FUICommandList);
	{
		ExtenderCommands->MapAction(
			FScriptEditorCommands::Get().APIBroswer,
			FExecuteAction::CreateSP(this, &SCodeEditableText::OpenAPIBrowser),
			FCanExecuteAction::CreateSP(this,&SCodeEditableText::CanOpenAPIBrowser));
	}

}

void SCodeEditableText::GoToLineColumn(int32 Line, int32 Column)
{
	FTextLocation Location(Line, Column);
	//
	GoTo(Location);
	ScrollTo(Location);

	US_Log("GoTo Line:%d,Column:%d", Line, Column);
}


void SCodeEditableText::GetLineAndColumn(int32 & Line, int32 & Column)
{
	Line = CurrentLine;
	Column = CurrentColumn;
}

void SCodeEditableText::ContextMenuExtender(FMenuBuilder& MenuBuilder)
{
	//MenuBuilder.AddSubMenu();
	//MenuBuilder.AddMenuSeparator(FName("CodeHelper"));

	MenuBuilder.PushCommandList(ExtenderCommands.ToSharedRef());

	MenuBuilder.BeginSection("CodeHelper", LOCTEXT("Heading", "Code Helper"));
	{
		MenuBuilder.AddMenuEntry(FScriptEditorCommands::Get().APIBroswer);
	}
	MenuBuilder.EndSection();

}

void SCodeEditableText::OnGraphActionMenuClosed(bool bActionExecuted, bool bContextSensitiveChecked, bool bGraphPinContext)
{

}

void SCodeEditableText::OpenAPIBrowser()
{
	US_Log("OpenAPIBrowser");

	TSharedRef<SScriptActionMenu> ActionMenu =
		SNew(SScriptActionMenu, FScriptEditor::Get())
		.CodeEditableObj(this)
		.NewNodePosition(CurrentMouseRightUpSSPosition)
		.AutoExpandActionMenu(true)
		.OnCloseReason(this, &SCodeEditableText::OnGraphActionMenuClosed);

	FActionMenuContent FocusedContent = FActionMenuContent(ActionMenu, ActionMenu->GetFilterTextBox());

	TSharedRef<SWidget> MenuContent = FocusedContent.Content;

	TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
		AsShared(),
		FWidgetPath(),
		MenuContent,
		CurrentMouseRightUpSSPosition,
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);

	if (Menu.IsValid() && Menu->GetOwnedWindow().IsValid())
	{
		Menu->GetOwnedWindow()->SetWidgetToFocusOnActivate(FocusedContent.WidgetToFocus);
	}

	if (Menu.IsValid())
	{
		Menu->GetOnMenuDismissed().AddLambda([DelegateList = FocusedContent.OnMenuDismissed](TSharedRef<IMenu>) { DelegateList.Broadcast(); });
	}
	else
	{
		FocusedContent.OnMenuDismissed.Broadcast();
	}
}

bool SCodeEditableText::CanOpenAPIBrowser() const
{
	return true;
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
	if (Character == TEXT('\t'))
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
		Reply = SMultiLineEditableText::OnKeyChar(MyGeometry, InCharacterEvent);
	}

	return Reply;
}

FReply SCodeEditableText::OnKeyDown(const FGeometry &Geometry, const FKeyEvent &KeyEvent)
{
	if (IsTextReadOnly()) { return FReply::Unhandled(); }
	FKey Key = KeyEvent.GetKey();

	if (Key == EKeys::Tab) {
		FString Selected;
		Selected.AppendChar(TEXT('\t'));
		//
		if (!GetSelectedText().IsEmpty()) {
			Selected.Append(GetSelectedText().ToString());
			Selected.ReplaceInline(TEXT("\n"), TEXT("\n\t"));
			Selected.RemoveFromEnd(TEXT("\t"));
			Selected.AppendChar(TEXT('\n'));
			InsertTextAtCursor(Selected);
		}///
		//
		return FReply::Handled();
	}///
	return SMultiLineEditableText::OnKeyDown(Geometry, KeyEvent);
}

FReply SCodeEditableText::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		CurrentMouseRightUpSSPosition = MouseEvent.GetScreenSpacePosition();
	}
	else if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		CurrentMouseLeftUpSSPosition = MouseEvent.GetScreenSpacePosition();
	}

	return SMultiLineEditableText::OnMouseButtonUp(MyGeometry, MouseEvent);
}

#undef LOCTEXT_NAMESPACE