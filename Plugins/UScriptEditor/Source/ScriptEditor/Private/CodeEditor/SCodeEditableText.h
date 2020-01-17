// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScriptEditorType.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Runtime/InputCore/Classes/InputCoreTypes.h"
#include "Widgets/Text/SlateEditableTextLayout.h"

DECLARE_DELEGATE(FOnInvokeSearchEvent);
DECLARE_DELEGATE_OneParam(FOnAutoCompleteEvent, const FString &);

class ITextLayoutMarshaller;

class SCodeEditableText : public SMultiLineEditableText
{
public:
	SLATE_BEGIN_ARGS( SCodeEditableText )
	{}
		/** The initial text that will appear in the widget. */
		SLATE_ATTRIBUTE(FText, Text)

		/** The marshaller used to get/set the raw text to/from the text layout. */
		SLATE_ARGUMENT(TSharedPtr< ITextLayoutMarshaller >, Marshaller)

		/** The horizontal scroll bar widget */
		SLATE_ARGUMENT(TSharedPtr< SScrollBar >, HScrollBar)

		/** The vertical scroll bar widget */
		SLATE_ARGUMENT(TSharedPtr< SScrollBar >, VScrollBar)

		SLATE_ATTRIBUTE(bool, CanKeyboardFocus)

		SLATE_ATTRIBUTE(bool, IsReadOnly)

		/** Called whenever the text is changed interactively by the user */
		SLATE_EVENT(FOnAutoCompleteEvent, OnAutoComplete)
		SLATE_EVENT(FOnInvokeSearchEvent, OnInvokeSearch)
		SLATE_EVENT(FOnTextChanged, OnTextChanged)
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)
		SLATE_EVENT(FMenuExtensionDelegate, ContextMenuExtender)

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	//
	//
	void SelectLine();
	void DeleteSelectedText();
	void GoToLineColumn(int32 Line, int32 Column);
	void GetLineAndColumn(int32 & Line, int32 & Column);

	void SetReferenceInfo(FScriptReferenceInfo& InInfo);

	const FTextLocation &GetCursorLocation() const;
	const FString GetUnderCursor() const;

	void OnCursorMoved(const FTextLocation & Location) {
		CurrentLine = Location.GetLineIndex();
		CurrentColumn = Location.GetOffset();

		CursorLocation = Location;
	}

	void ContextMenuExtender(class FMenuBuilder& MenuBuilder);

protected:

	void OnGraphActionMenuClosed(bool bActionExecuted, bool bContextSensitiveChecked, bool bGraphPinContext);

	void OnAutoCompleteMenuSelectedCode(const FString& InCode);

	void OpenAPIBrowser();

	void OpenAutoCompleteMenu(FString InKeywork);

	bool PushKeyword(FString InKeywork,bool InContext = false);

	bool InsertCompleteKeywork();

	bool CanOpenAPIBrowser()const;

	bool IsAutoCompleteMenuOpen()const;

	FOnInvokeSearchEvent OnInvokedSearch;
	FOnAutoCompleteEvent OnAutoCompleted;

	FString AutoCompleteResults;

	FTextLocation CursorLocation;
	FVector2D CursorScreenLocation;

	FString UnderCursor;
	bool KeyboardFocus;

	FScriptReferenceInfo ReferenceInfo;
private:
	void AutoCleanup(FString &Keyword);

	virtual FReply OnKeyChar(const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent) override;
	virtual FReply OnKeyDown(const FGeometry &Geometry, const FKeyEvent &KeyEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual void Tick(const FGeometry &AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

	int32 CurrentLine;
	int32 CurrentColumn;

	FVector2D CurrentMouseRightUpSSPosition;
	FVector2D CurrentMouseLeftUpSSPosition;

	FString CurrentKeyword;

	TSharedPtr<class SAutoCompleteMenu> AutoCompleteMenu;
	TSharedPtr<class FUICommandList> ExtenderCommands;
};
