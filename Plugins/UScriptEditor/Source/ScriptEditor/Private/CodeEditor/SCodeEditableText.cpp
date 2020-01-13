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
#include "SAutoCompleteMenu.h"

#define  LOCTEXT_NAMESPACE "CodeEditableText"


void SCodeEditableText::Construct(const FArguments& InArgs)
{
	//
	OnAutoCompleted = InArgs._OnAutoComplete;
	OnInvokedSearch = InArgs._OnInvokeSearch;
	KeyboardFocus = InArgs._CanKeyboardFocus.Get();
	//

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
			FCanExecuteAction::CreateSP(this, &SCodeEditableText::CanOpenAPIBrowser));
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

const FTextLocation & SCodeEditableText::GetCursorLocation() const
{
	return CursorLocation;
}

const FString SCodeEditableText::ParseAutoCompleteWord()
{
	if (CursorLocation.GetOffset() <= INDEX_NONE + 1) { return FString(); }
	//
	FTextLocation Offset(CursorLocation.GetLineIndex(), CursorLocation.GetOffset());
	TCHAR IT = EditableTextLayout->GetCharacterAt(Offset);
	int32 I = Offset.GetOffset();
	//
	FString Subject;
	FString Raw;
	//
	while ((I > INDEX_NONE) && (!TChar<WIDECHAR>::IsWhitespace(IT)) && (!TChar<WIDECHAR>::IsLinebreak(IT)) && (TChar<WIDECHAR>::IsAlpha(IT) || TChar<WIDECHAR>::IsDigit(IT)
		|| IT == TEXT('.') || IT == TEXT(':') || IT == TEXT('-') || IT == TEXT('_') || IT == TEXT('<') || IT == TEXT('>') || IT == TEXT('{') || IT == TEXT('}') || IT == TEXT('[') || IT == TEXT(']') || IT == TEXT('(') || IT == TEXT(')')
		)) {
		Offset = FTextLocation(CursorLocation.GetLineIndex(), I);
		IT = EditableTextLayout->GetCharacterAt(Offset);
		Raw.AppendChar(IT);
		--I;
	}
	//
	Subject = Raw.TrimStartAndEnd();
	Subject.ReverseString();
	//
	if (Subject.Contains(TEXT("::"))) { Subject.Split(TEXT("::"), nullptr, &Subject, ESearchCase::IgnoreCase, ESearchDir::FromEnd); }
	if (Subject.Contains(TEXT("."))) { Subject.Split(TEXT("."), nullptr, &Subject, ESearchCase::IgnoreCase, ESearchDir::FromEnd); }
	//
	Subject.Split(TEXT("{"), &Subject, nullptr); Subject.Split(TEXT("("), &Subject, nullptr);
	Subject.Split(TEXT("["), &Subject, nullptr); Subject.Split(TEXT("<"), &Subject, nullptr);
	//
	AutoCleanup(Subject);
	//
	return Subject;
}

const FString SCodeEditableText::GetUnderCursor() const
{
	return UnderCursor;
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

void SCodeEditableText::OpenAutoCompleteMenu(FString InKeywork)
{
	US_Log("OpenAutoCompleteMenu");

	TSharedRef<SAutoCompleteMenu> AutoMenu =
		SNew(SAutoCompleteMenu, FScriptEditor::Get())
		.CodeEditableObj(this)
		.NewNodePosition(CursorScreenLocation)
		.AutoExpandActionMenu(true);


	FActionMenuContent FocusedContent = FActionMenuContent(AutoMenu, AutoMenu->GetFilterTextBox());

	TSharedRef<SWidget> MenuContent = FocusedContent.Content;

	AutoMenu->SetFilterText(FText::FromString(InKeywork));
	if (AutoMenu->IsMatchingAny())
	{
		TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
			AsShared(),
			FWidgetPath(),
			MenuContent,
			CursorScreenLocation,
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu),
			false
		);


// 		if (Menu.IsValid())
// 		{
// 			Menu->GetOnMenuDismissed().AddLambda([DelegateList = FocusedContent.OnMenuDismissed](TSharedRef<IMenu>) { DelegateList.Broadcast(); });
// 		}
// 		else
// 		{
// 			FocusedContent.OnMenuDismissed.Broadcast();
// 		}
	}

}

bool SCodeEditableText::PushKeyword(FString InKeywork)
{
	TSharedPtr<SWindow> MenuWindow = FSlateApplication::Get().GetVisibleMenuWindow();
	
	if (MenuWindow.IsValid())
	{	
		const auto& MenuWidget = StaticCastSharedPtr<SWidget>(MenuWindow);
		const auto& Menu = StaticCastSharedPtr<SAutoCompleteMenu>(MenuWidget);
		if (Menu.IsValid())
		{
 			Menu->SetFilterText(FText::FromString(InKeywork));
			//Menu->SetRenderTransformPivot();
 			if (!Menu->IsMatchingAny())
 			{
 				FSlateApplication::Get().DismissAllMenus();
 			}		
		}
		else
		{
			FSlateApplication::Get().DismissAllMenus();
		}
	}
	else
	{
		OpenAutoCompleteMenu(InKeywork);
	}
	return true;
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

void SCodeEditableText::AutoCleanup(FString &Keyword)
{
	FString Clean;
	//
	for (const TCHAR &CH : Keyword) {
		if ((TChar<WIDECHAR>::IsAlpha(CH) || TChar<WIDECHAR>::IsDigit(CH) || CH == TEXT('_'))) { Clean.AppendChar(CH); }
	}///
	//
	Keyword.Empty();
	Keyword.Append(Clean);
}

void SCodeEditableText::AutoCompleteWord(const int32 X)
{
	if (CursorLocation.GetOffset() - X <= INDEX_NONE) { return; }
	//
	FTextLocation Offset(CursorLocation.GetLineIndex(), CursorLocation.GetOffset() - X);
	TCHAR IT = EditableTextLayout->GetCharacterAt(Offset);
	int32 I = Offset.GetOffset();
	AutoCompleteResults.Empty();
	//
	FString Subject;
	FString Parent;
	FString Raw;
	//
	while ((I > INDEX_NONE) && (!TChar<WIDECHAR>::IsWhitespace(IT)) && (!TChar<WIDECHAR>::IsLinebreak(IT)) && (TChar<WIDECHAR>::IsAlpha(IT) || TChar<WIDECHAR>::IsDigit(IT)
		|| IT == TEXT('.') || IT == TEXT(':') || IT == TEXT('-') || IT == TEXT('_') || IT == TEXT('<') || IT == TEXT('>') || IT == TEXT('{') || IT == TEXT('}') || IT == TEXT('[') || IT == TEXT(']') || IT == TEXT('(') || IT == TEXT(')')
		)) {
		Offset = FTextLocation(CursorLocation.GetLineIndex(), I);
		IT = EditableTextLayout->GetCharacterAt(Offset);
		Raw.AppendChar(IT);
		--I;
	}
	//
	Subject = Raw.TrimStartAndEnd();
	Subject.ReverseString();
	//
	if (Subject.Contains(TEXT("::"))) { Subject.Split(TEXT("::"), &Parent, &Subject, ESearchCase::IgnoreCase, ESearchDir::FromEnd); }
	if (Subject.Contains(TEXT("."))) { Subject.Split(TEXT("."), &Parent, &Subject, ESearchCase::IgnoreCase, ESearchDir::FromEnd); }
	//
	if (Parent.Contains(TEXT("::"))) { Parent.Split(TEXT("::"), nullptr, &Parent, ESearchCase::IgnoreCase, ESearchDir::FromEnd); }
	if (Parent.Contains(TEXT("."))) { Parent.Split(TEXT("."), nullptr, &Parent, ESearchCase::IgnoreCase, ESearchDir::FromEnd); }
	//
	Parent.Split(TEXT("{"), &Parent, nullptr); Parent.Split(TEXT("("), &Parent, nullptr);
	Parent.Split(TEXT("["), &Parent, nullptr); Parent.Split(TEXT("<"), &Parent, nullptr);
	//
	Subject.Split(TEXT("{"), &Subject, nullptr); Subject.Split(TEXT("("), &Subject, nullptr);
	Subject.Split(TEXT("["), &Subject, nullptr); Subject.Split(TEXT("<"), &Subject, nullptr);
	//
	AutoCleanup(Parent);
	AutoCleanup(Subject);
	//
	//IKMGC_ScriptParser::AutoComplete(Parent, Subject, AutoCompleteResults);
	OnAutoCompleted.ExecuteIfBound(AutoCompleteResults);
}

FReply SCodeEditableText::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	FReply Reply = FReply::Unhandled();
	bool PushCursorMenu = false;
	//////////////////////////////////////////////////////////////////////////
	//TODO:
	int32 FontHigh = 17,FontWide = 9;
	CursorScreenLocation = FVector2D(MyGeometry.GetAbsolutePosition().X + GetCursorLocation().GetOffset()* FontWide, MyGeometry.GetAbsolutePosition().Y + (GetCursorLocation().GetLineIndex() + 1)*FontHigh);
	//////////////////////////////////////////////////////////////////////////

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
	else if (Character == TEXT('.'))
	{
		if (!IsTextReadOnly())
		{
			if (CursorLocation.GetOffset() - 1 > INDEX_NONE)
			{
				AutoCompleteWord(1);
			}///
			//
			FString DOT;
			DOT.AppendChar(Character);
			InsertTextAtCursor(DOT);
			//
			Reply = FReply::Handled();
		}
		else
		{
			Reply = FReply::Unhandled();
		}
	}
	else if (Character == TEXT(':'))
	{
		if (!IsTextReadOnly())
		{
			if (CursorLocation.GetOffset() - 2 > INDEX_NONE)
			{
				FTextLocation Offset(CursorLocation.GetLineIndex(), CursorLocation.GetOffset() - 1);
				TCHAR AR = EditableTextLayout->GetCharacterAt(Offset);
				if (AR == TEXT(':')) { AutoCompleteWord(2); }
			}///
			//
			FString DOT;
			DOT.AppendChar(Character);
			InsertTextAtCursor(DOT);
			//

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
		FString tCursor = UnderCursor;
		if ((Character >= TEXT('a') && Character <= TEXT('z')) || (Character >= TEXT('A') && Character <= TEXT('Z')))
		{		
			tCursor.AppendChar(Character);
			US_Log("AutoCompleteResults:%s", *tCursor);
			PushKeyword(tCursor);
			PushCursorMenu = true;
		}
		else if (Character == TEXT('\b') && UnderCursor.Len() > 1)
		{
			tCursor.RemoveAt(tCursor.Len() - 1);
			US_Log("AutoCompleteResults:%s", *tCursor);
			PushKeyword(tCursor);
			PushCursorMenu = true;
		}

	}


	if (!PushCursorMenu)
	{
		FSlateApplication::Get().DismissAllMenus();
	}

	return Reply;
}

FReply SCodeEditableText::OnKeyDown(const FGeometry &Geometry, const FKeyEvent &KeyEvent)
{
	if (IsTextReadOnly()) { return FReply::Unhandled(); }
	FKey Key = KeyEvent.GetKey();

	if ((Key == EKeys::F) && (KeyEvent.IsCommandDown() || KeyEvent.IsControlDown())) {
		if (OnInvokedSearch.IsBound()) {
			OnInvokedSearch.Execute();
		} return FReply::Handled();
	}///
	if ((Key == EKeys::G) && (KeyEvent.IsCommandDown() || KeyEvent.IsControlDown())) {
		//TODO:Show
		CurrentMouseRightUpSSPosition = CurrentMouseLeftUpSSPosition;
		OpenAPIBrowser();
		return FReply::Handled();
	}///


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

	if ((KeyEvent.GetKey() == EKeys::Escape || KeyEvent.GetKey() == EKeys::BackSpace) && (AutoCompleteResults.Num() >= 1)) {
		AutoCompleteResults.Empty();
		OnAutoCompleted.ExecuteIfBound(AutoCompleteResults);
		//
		return FReply::Handled();
	}///
	//
	//if (Key == EKeys::Enter) { return FReply::Handled(); }
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

FCursorReply SCodeEditableText::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return SMultiLineEditableText::OnCursorQuery(MyGeometry, CursorEvent);
}

void SCodeEditableText::Tick(const FGeometry &AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	EditableTextLayout->Tick(AllottedGeometry, CurrentTime, DeltaTime);
	//
	TSharedPtr<const IRun>Run = EditableTextLayout->GetRunUnderCursor();
	if (Run.IsValid() && CursorLocation.IsValid() && GetSelectedText().IsEmpty()) {
		FString Under; Run->AppendTextTo(Under);
		if (!Under.Equals(UnderCursor)) { UnderCursor = Under; }
	}///
}

#undef LOCTEXT_NAMESPACE