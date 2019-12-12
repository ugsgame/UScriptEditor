// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "ScriptEditorStyle.h"
#include "Framework/Text/SyntaxTokenizer.h"
#include "Framework/Text/SyntaxHighlighterTextLayoutMarshaller.h"

class FTextLayout;

/**
 * Get/set the raw text to/from a text layout, and also inject syntax highlighting for our rich-text markup
 */
class FLUARichTextSyntaxHighlighterTextLayoutMarshaller : public FSyntaxHighlighterTextLayoutMarshaller
{
public:

	struct FSyntaxTextStyle
	{
		FSyntaxTextStyle()
			: NormalTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.Normal"))
			, OperatorTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.Operator"))
			, KeywordTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.Keyword"))
			, StringTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.String"))
			, NumberTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.Number"))
			, CommentTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.Comment"))
			, PreProcessorKeywordTextStyle(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("SyntaxHighlight.LUA.PreProcessorKeyword"))
		{
		}

		FSyntaxTextStyle(const FTextBlockStyle& InNormalTextStyle, const FTextBlockStyle& InOperatorTextStyle, const FTextBlockStyle& InKeywordTextStyle, const FTextBlockStyle& InStringTextStyle, const FTextBlockStyle& InNumberTextStyle, const FTextBlockStyle& InCommentTextStyle, const FTextBlockStyle& InPreProcessorKeywordTextStyle)
			: NormalTextStyle(InNormalTextStyle)
			, OperatorTextStyle(InOperatorTextStyle)
			, KeywordTextStyle(InKeywordTextStyle)
			, StringTextStyle(InStringTextStyle)
			, NumberTextStyle(InNumberTextStyle)
			, CommentTextStyle(InCommentTextStyle)
			, PreProcessorKeywordTextStyle(InPreProcessorKeywordTextStyle)
		{
		}

		FTextBlockStyle NormalTextStyle;
		FTextBlockStyle OperatorTextStyle;
		FTextBlockStyle KeywordTextStyle;
		FTextBlockStyle StringTextStyle;
		FTextBlockStyle NumberTextStyle;
		FTextBlockStyle CommentTextStyle;
		FTextBlockStyle PreProcessorKeywordTextStyle;
	};

	static TSharedRef< FLUARichTextSyntaxHighlighterTextLayoutMarshaller > Create(const FSyntaxTextStyle& InSyntaxTextStyle);

	virtual ~FLUARichTextSyntaxHighlighterTextLayoutMarshaller();

protected:

	virtual void ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines) override;

	FLUARichTextSyntaxHighlighterTextLayoutMarshaller(TSharedPtr< FSyntaxTokenizer > InTokenizer, const FSyntaxTextStyle& InSyntaxTextStyle);

	/** Styles used to display the text */
	FSyntaxTextStyle SyntaxTextStyle;

	/** String representing tabs */
	FString TabString;
};
