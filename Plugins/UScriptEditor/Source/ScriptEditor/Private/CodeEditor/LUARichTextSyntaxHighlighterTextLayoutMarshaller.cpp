// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "LUARichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "Framework/Text/IRun.h"
#include "Framework/Text/TextLayout.h"
#include "Framework/Text/ISlateRun.h"
#include "Framework/Text/SlateTextRun.h"
#include "Misc/ExpressionParserTypes.h"
#include "WhiteSpaceTextRun.h"

#include "ScriptEditorUtils.h"

//TODO:

const TCHAR* LuaKeywords[] =
{
	TEXT("and"),
	TEXT("break"),
	TEXT("do"),
	TEXT("else"),
	TEXT("elseif"),
	TEXT("end"),
	TEXT("false"),
	TEXT("for"),
	TEXT("function"),
	TEXT("if"),
	TEXT("in"),
	TEXT("local"),
	TEXT("nil"),
	TEXT("not"),
	TEXT("or"),
	TEXT("repeat"),
	TEXT("return"),
	TEXT("then"),
	TEXT("true"),
	TEXT("until"),
	TEXT("while"),
	TEXT("goto"),
};

const TCHAR* LuaOperators[] =
{
	TEXT("--"),
	TEXT("[["), 	
	TEXT("]]"),		
	TEXT("\""),
	TEXT("\'"),
	TEXT("::"),
	TEXT(":"),
	TEXT("+"),
	TEXT("-"),
	TEXT("("),
	TEXT(")"),
 //	TEXT("["),
 //	TEXT("]"),
	TEXT("."),
	TEXT("~"),
	TEXT("~="),
	TEXT("*="),
	TEXT("*"),
	TEXT("/="),
	TEXT("/"),
	TEXT("%="),
	TEXT("%"),
	TEXT("<="),
	TEXT("<"),
	TEXT(">="),
	TEXT(">"),
	TEXT("=="),
	TEXT("^="),
	TEXT("^"),
	TEXT("="),
	TEXT(","),
	TEXT("{"),
	TEXT("}"),
	TEXT(";"),
	TEXT("#")
	TEXT("..")
};

const TCHAR* LuaPreProcessorKeywords[] =
{
	TEXT("#include"),
	TEXT("#define"),
	TEXT("#ifndef"),
	TEXT("#ifdef"),
	TEXT("#if"),
	TEXT("#else"),
	TEXT("#endif"),
	TEXT("#pragma"),
	TEXT("#undef"),
};

TSharedRef< FLUARichTextSyntaxHighlighterTextLayoutMarshaller > FLUARichTextSyntaxHighlighterTextLayoutMarshaller::Create(const FSyntaxTextStyle& InSyntaxTextStyle)
{
	TArray<FSyntaxTokenizer::FRule> TokenizerRules;

	// operators
	for(const auto& Operator : LuaOperators)
	{
		TokenizerRules.Emplace(FSyntaxTokenizer::FRule(Operator));
	}	

	// keywords
	for(const auto& Keyword : LuaKeywords)
	{
		TokenizerRules.Emplace(FSyntaxTokenizer::FRule(Keyword));
	}

	// Pre-processor Keywords
	for(const auto& PreProcessorKeyword : LuaPreProcessorKeywords)
	{
		TokenizerRules.Emplace(FSyntaxTokenizer::FRule(PreProcessorKeyword));
	}

	return MakeShareable(new FLUARichTextSyntaxHighlighterTextLayoutMarshaller(FSyntaxTokenizer::Create(TokenizerRules), InSyntaxTextStyle));
}

FLUARichTextSyntaxHighlighterTextLayoutMarshaller::~FLUARichTextSyntaxHighlighterTextLayoutMarshaller()
{

}

void FLUARichTextSyntaxHighlighterTextLayoutMarshaller::ParseTokens(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FSyntaxTokenizer::FTokenizedLine> TokenizedLines)
{
	enum class EParseState : uint8
	{
		None,
		LookingForString,
		LookingForCharacter,
		LookingForSingleLineComment,
		LookingForMultiLineComment,
	};

	TArray<FTextLayout::FNewLineData> LinesToAdd;
	LinesToAdd.Reserve(TokenizedLines.Num());

	// Parse the tokens, generating the styled runs for each line
	EParseState ParseState = EParseState::None;
	for(const FSyntaxTokenizer::FTokenizedLine& TokenizedLine : TokenizedLines)
	{
		TSharedRef<FString> ModelString = MakeShareable(new FString());
		TArray< TSharedRef< IRun > > Runs;

		if(ParseState == EParseState::LookingForSingleLineComment)
		{
			ParseState = EParseState::None;
		}

		for(const FSyntaxTokenizer::FToken& Token : TokenizedLine.Tokens)
		{
			const FString TokenText = SourceString.Mid(Token.Range.BeginIndex, Token.Range.Len());

			const FTextRange ModelRange(ModelString->Len(), ModelString->Len() + TokenText.Len());
			ModelString->Append(TokenText);

			FRunInfo RunInfo(TEXT("SyntaxHighlight.LUA.Normal"));
			FTextBlockStyle TextBlockStyle = SyntaxTextStyle.NormalTextStyle;

			const bool bIsWhitespace = FString(TokenText).TrimEnd().IsEmpty();
			if(!bIsWhitespace)
			{
				bool bHasMatchedSyntax = false;
				if(Token.Type == FSyntaxTokenizer::ETokenType::Syntax)
				{
					if(ParseState == EParseState::None && TokenText == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::LookingForString;
						bHasMatchedSyntax = true;
					}
					else if(ParseState == EParseState::LookingForString && TokenText == TEXT("\""))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Normal");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TokenText == TEXT("\'"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::LookingForCharacter;
						bHasMatchedSyntax = true;
					}
					else if(ParseState == EParseState::LookingForCharacter && TokenText == TEXT("\'"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Normal");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TokenText.StartsWith(TEXT("#")))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.PreProcessorKeyword");
						TextBlockStyle = SyntaxTextStyle.PreProcessorKeywordTextStyle;
						ParseState = EParseState::None;
					}
					else if (ParseState == EParseState::None && TokenText == TEXT("--"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::LookingForSingleLineComment;
					}
					else if(ParseState == EParseState::LookingForSingleLineComment && TokenText == TEXT("[["))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::LookingForMultiLineComment;
					}
					else if(ParseState == EParseState::LookingForMultiLineComment && TokenText == TEXT("]]"))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && TChar<WIDECHAR>::IsAlpha(TokenText[0]))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Keyword");
						TextBlockStyle = SyntaxTextStyle.KeywordTextStyle;
						ParseState = EParseState::None;
					}
					else if(ParseState == EParseState::None && !TChar<WIDECHAR>::IsAlpha(TokenText[0]))
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Operator");
						TextBlockStyle = SyntaxTextStyle.OperatorTextStyle;
						ParseState = EParseState::None;
					}
				}
				
				// It's possible that we fail to match a syntax token if we're in a state where it isn't parsed
				// In this case, we treat it as a literal token
				if(Token.Type == FSyntaxTokenizer::ETokenType::Literal || !bHasMatchedSyntax)
				{
					if(ParseState == EParseState::LookingForString)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
					}
					else if(ParseState == EParseState::LookingForCharacter)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.String");
						TextBlockStyle = SyntaxTextStyle.StringTextStyle;
					}
					else if(ParseState == EParseState::LookingForSingleLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					}
					else if(ParseState == EParseState::LookingForMultiLineComment)
					{
						RunInfo.Name = TEXT("SyntaxHighlight.LUA.Comment");
						TextBlockStyle = SyntaxTextStyle.CommentTextStyle;
					}
				}

				TSharedRef< ISlateRun > Run = FSlateTextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange);
				Runs.Add(Run);
			}
			else
			{
				RunInfo.Name = TEXT("SyntaxHighlight.LUA.WhiteSpace");
				TSharedRef< ISlateRun > Run = FWhiteSpaceTextRun::Create(RunInfo, ModelString, TextBlockStyle, ModelRange, 4);
				Runs.Add(Run);
			}
		}

		LinesToAdd.Emplace(MoveTemp(ModelString), MoveTemp(Runs));
	}

	TargetTextLayout.AddLines(LinesToAdd);
}

FLUARichTextSyntaxHighlighterTextLayoutMarshaller::FLUARichTextSyntaxHighlighterTextLayoutMarshaller(TSharedPtr< FSyntaxTokenizer > InTokenizer, const FSyntaxTextStyle& InSyntaxTextStyle)
	: FSyntaxHighlighterTextLayoutMarshaller(MoveTemp(InTokenizer))
	, SyntaxTextStyle(InSyntaxTextStyle)
{
}
