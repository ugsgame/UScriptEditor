// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Runtime/InputCore/Classes/InputCoreTypes.h"
#include "Widgets/Text/SlateEditableTextLayout.h"

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
		//SLATE_EVENT(FOnAutoCompleteEvent, OnAutoComplete)
		//SLATE_EVENT(FOnInvokeSearchEvent, OnInvokeSearch)
		SLATE_EVENT(FOnTextChanged, OnTextChanged)
		SLATE_EVENT(FOnTextCommitted, OnTextCommitted)

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	//
	//
	void SelectLine();
	void DeleteSelectedText();
	void GoToLineColumn(int32 Line, int32 Column);
	void GetLineAndColumn(int32 & Line, int32 & Column);

	void OnCursorMoved(const FTextLocation & Location) {
		CurrentLine = Location.GetLineIndex();
		CurrentColumn = Location.GetOffset();
	}
private:
	virtual FReply OnKeyChar(const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent) override;

	int32 CurrentLine;
	int32 CurrentColumn;
};
