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
#include "ScriptSchemaAction.h"

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

	CurParseType = ECompleteParseType::None;
	PerParseType = ECompleteParseType::None;
}

void SCodeEditableText::GoToLineColumn(int32 Line, int32 Column)
{
	FTextLocation Location(Line, Column);
	//
	GoTo(Location);
	ScrollTo(Location);

	US_Log("GoTo Line:%d,Column:%d", Line, Column);
}


void SCodeEditableText::GetLineAndColumn(int32& Line, int32& Column)
{
	Line = CurrentLine;
	Column = CurrentColumn;
}

void SCodeEditableText::SetReferenceInfo(FScriptReferenceInfo& InInfo)
{
	ReferenceInfo = InInfo;
	US_Log("Refrence:%s", *ReferenceInfo.ReferencedAsset.ToString());
}

const FTextLocation& SCodeEditableText::GetCursorLocation() const
{
	return CursorLocation;
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

void SCodeEditableText::OnAutoCompleteMenuSelectedCode(const FString& InCode)
{
	AutoCleanup(CurrentKeyword);
	InsertTextAtCursor(InCode);
}

void SCodeEditableText::OpenAPIBrowser()
{
	US_Log("OpenAPIBrowser");

	TSharedRef<SScriptActionMenu> ActionMenu =
		SNew(SScriptActionMenu, FScriptEditor::Get())
		.CodeEditableObj(this)
		.NewNodePosition(CurrentMouseRightUpSSPosition)
		.AutoExpandActionMenu(true)
		.ReferenceInfo(ReferenceInfo)
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

void SCodeEditableText::OpenAutoCompleteMenu(FString InKeywork,ECompleteParseType InParse,bool InContext)
{
	//US_Log("OpenAutoCompleteMenu");
	AutoCompleteMenu =
		SNew(SAutoCompleteMenu, FScriptEditor::Get())
		.CodeEditableObj(this)
		.NewNodePosition(CursorScreenLocation)
		.AutoExpandActionMenu(true)
		.ReferenceInfo(ReferenceInfo)
		.ParseType(InParse)
		.SelfContext(InContext)
		.OnActionCodeSelected(this, &SCodeEditableText::OnAutoCompleteMenuSelectedCode);

	FActionMenuContent FocusedContent = FActionMenuContent(AutoCompleteMenu.ToSharedRef(), AutoCompleteMenu->GetFilterTextBox());

	TSharedRef<SWidget> MenuContent = FocusedContent.Content;

	AutoCompleteMenu->SetFilterText(FText::FromString(InKeywork));

	if (AutoCompleteMenu->IsMatchingAny())
	{
		TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
			AsShared(),
			FWidgetPath(),
			MenuContent,
			CursorScreenLocation,
			FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu),
			false
		);

		if (Menu.IsValid())
		{
			Menu->GetOnMenuDismissed().AddLambda([DelegateList = FocusedContent.OnMenuDismissed](TSharedRef<IMenu>) { DelegateList.Broadcast(); });
		}
		else
		{
			FocusedContent.OnMenuDismissed.Broadcast();
		}
	}

}

bool SCodeEditableText::PushKeyword(FString InKeywork, ECompleteParseType InParse)
{
	bool SelfContext = false;
	bool ShowAll = false;

	//Check Self 
	if (InKeywork == FString("self") || InKeywork == FString("self:")|| InKeywork == FString("self."))
	{
		InKeywork = "";
		SelfContext = true;
	}
	else if (InKeywork.Contains("self"))
	{
		SelfContext = InKeywork.RemoveFromStart("self:");
		if (!SelfContext)
		{
			SelfContext = InKeywork.RemoveFromStart("self.");
		}
	}
	//Check Dot
	if (!SelfContext)
	{
		
	}
	//
	US_Log("InKeywork:%s,%d", *InKeywork, InParse);
	//
	CurrentKeyword = InKeywork;
	FSlateApplication::Get().DismissAllMenus();
	//
	if (/*InKeywork.Len() < 2 ||*/ InKeywork.Len() > 30)
	{
		return false;
	}
	//
	if (PerParseType != InParse)
	{
		PerParseType = InParse;
		OpenAutoCompleteMenu(InKeywork, InParse, SelfContext);
		return true;
	}
	else
	{
		PerParseType = InParse;
		//
		if (IsAutoCompleteMenuOpen() && AutoCompleteMenu.IsValid())
		{
			AutoCompleteMenu->SetFilterText(FText::FromString(InKeywork));

			if (AutoCompleteMenu->IsMatchingAny())
			{
				//Update AutoCompleteMenu position
				TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(
					AsShared(),
					FWidgetPath(),
					AutoCompleteMenu.ToSharedRef(),
					CursorScreenLocation,
					FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu),
					false
				);
			}
			else
			{
				return false;
			}
		}
		else
		{
			OpenAutoCompleteMenu(InKeywork, InParse, SelfContext);
		}
		return true;
	}
}

bool SCodeEditableText::InsertCompleteKeywork()
{
	if (IsAutoCompleteMenuOpen() && AutoCompleteMenu.IsValid())
	{
		FSlateApplication::Get().DismissAllMenus();

		TArray< TSharedPtr<FEdGraphSchemaAction> >SelectedActions;
		AutoCompleteMenu->GetSelectedActions(SelectedActions);

		if (SelectedActions.Num() > 0)
		{
			FScriptSchemaAction* ScriptAction = static_cast<FScriptSchemaAction*>(SelectedActions[0].Get());
			AutoCleanup(CurrentKeyword);
			this->InsertTextAtCursor(ScriptAction->CodeClip);
			OnAutoCompleted.ExecuteIfBound(CurrentKeyword);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool SCodeEditableText::CanOpenAPIBrowser() const
{
	return true;
}

bool SCodeEditableText::IsAutoCompleteMenuOpen() const
{
	TSharedPtr<SWindow> MenuWindow = FSlateApplication::Get().GetVisibleMenuWindow();
	if (MenuWindow.IsValid())
	{
		const auto& MenuWidget = StaticCastSharedPtr<SWidget>(MenuWindow);
		const auto& Menu = StaticCastSharedPtr<SAutoCompleteMenu>(MenuWidget);
		if (Menu.IsValid())
		{
			return true;
		}
	}
	return false;
}

void SCodeEditableText::SelectLine() {
	EditableTextLayout->JumpTo(ETextLocation::EndOfLine, ECursorAction::MoveCursor);
	EditableTextLayout->JumpTo(ETextLocation::BeginningOfLine, ECursorAction::SelectText);
}

void SCodeEditableText::DeleteSelectedText() {
	EditableTextLayout->DeleteSelectedText();
}

void SCodeEditableText::AutoCleanup(FString& Keyword)
{
	for (int32 i = 0; i < Keyword.Len(); i++)
	{
		EditableTextLayout->HandleBackspace();
	}
}

FReply SCodeEditableText::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	FReply Reply = FReply::Unhandled();
	bool PushCursorMenu = false;

	if (IsTextReadOnly())return Reply;
	//////////////////////////////////////////////////////////////////////////
	//TODO:
	int32 FontHigh = 17, FontWide = 9;
	CursorScreenLocation = FVector2D(MyGeometry.GetAbsolutePosition().X + GetCursorLocation().GetOffset() * FontWide, MyGeometry.GetAbsolutePosition().Y + (GetCursorLocation().GetLineIndex() + 1) * FontHigh);
	//////////////////////////////////////////////////////////////////////////

	const TCHAR Character = InCharacterEvent.GetCharacter();

	if (Character == TEXT('\t'))
	{

		if (!InsertCompleteKeywork())
		{
			FString String;
			String.AppendChar(Character);
			InsertTextAtCursor(String);
			Reply = FReply::Handled();
		}
		CurParseType = ECompleteParseType::None;
	}
	else if (Character == TEXT('.'))
	{
		//TODO:
		FString DOT;
		DOT.AppendChar(Character);
		InsertTextAtCursor(DOT);

		CurParseType = ECompleteParseType::Dot;
		Reply = FReply::Handled();
	}
	else if (Character == TEXT(':'))
	{
		//TODO:
		FString DOT;
		DOT.AppendChar(Character);
		InsertTextAtCursor(DOT);

		CurParseType = ECompleteParseType::Colon;
		Reply = FReply::Handled();
	}
	else
	{
		FString CursorString = UnderCursor;
		if ((Character >= TEXT('a') && Character <= TEXT('z')) || (Character >= TEXT('A') && Character <= TEXT('Z')))
		{
			CursorString.AppendChar(Character);
			PushKeyword(CursorString, CurParseType);
			PushCursorMenu = true;
		}
		else if (Character == TEXT('\b') && UnderCursor.Len() > 1)
		{
			CursorString.RemoveAt(CursorString.Len() - 1);
			PushKeyword(CursorString, CurParseType);
			PushCursorMenu = true;
		}
		else
		{
			CurParseType = ECompleteParseType::None;
		}

		Reply = SMultiLineEditableText::OnKeyChar(MyGeometry, InCharacterEvent);
	}

	if (!PushCursorMenu)
	{
		FSlateApplication::Get().DismissAllMenus();
	}

	return Reply;
}

FReply SCodeEditableText::OnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (IsTextReadOnly()) { return FReply::Unhandled(); }
	FKey Key = KeyEvent.GetKey();

	if ((Key == EKeys::F) && (KeyEvent.IsCommandDown() || KeyEvent.IsControlDown())) {
		if (OnInvokedSearch.IsBound()) {
			OnInvokedSearch.Execute();
		} return FReply::Handled();
	}

	if ((Key == EKeys::G) && (KeyEvent.IsCommandDown() || KeyEvent.IsControlDown())) {
		//Show API Browser
		CurrentMouseRightUpSSPosition = CurrentMouseLeftUpSSPosition;
		OpenAPIBrowser();
		return FReply::Handled();
	}

	if (Key == EKeys::Tab)
	{
		FString Selected;
		Selected.AppendChar(TEXT('\t'));

		if (!GetSelectedText().IsEmpty()) {
			Selected.Append(GetSelectedText().ToString());
			Selected.ReplaceInline(TEXT("\n"), TEXT("\n\t"));
			Selected.RemoveFromEnd(TEXT("\t"));
			Selected.AppendChar(TEXT('\n'));
			InsertTextAtCursor(Selected);
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Enter)
	{
		if (InsertCompleteKeywork())
		{
			return FReply::Handled();
		}
	}

	if (Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Right || Key == EKeys::Left)
	{
		if (IsAutoCompleteMenuOpen())
		{
			FReply Reply = FReply::Handled();
			AutoCompleteMenu->SetUserFoucs(Reply);
			return Reply;
		}
	}

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
	//DismissAllMenus
	FSlateApplication::Get().DismissAllMenus();

	return SMultiLineEditableText::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FCursorReply SCodeEditableText::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return SMultiLineEditableText::OnCursorQuery(MyGeometry, CursorEvent);
}

void SCodeEditableText::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
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