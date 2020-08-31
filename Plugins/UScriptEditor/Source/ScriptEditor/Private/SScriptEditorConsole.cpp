// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SScriptEditorConsole.h"
//#include "PythonConsoleModule.h"
#include "SScriptEditorConsole.h"
#include "SScriptEditorLog.h"

namespace ScriptEditorConsoleDefs
{
	// How many seconds to animate when console is summoned
	static const float IntroAnimationDuration = 0.25f;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SScriptEditorConsole::Construct( const FArguments& InArgs, const EScriptEditorConsoleStyle::Type InStyle, /*FPythonConsoleModule* PythonConsoleModule,*/ const FScriptEditorConsoleDelegates* PythonConsoleDelegates )
{
	CurrentStyle = InStyle;

	TSharedPtr<SScriptConsoleInputBox> ConsoleInputBox;

	//check( PythonConsoleModule != NULL );
	ChildSlot
	[ 
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SVerticalBox )
				.Visibility( this, &SScriptEditorConsole::MakeVisibleIfLogIsShown )
					
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 10.0f )
				[
					SNew(SBox)
					.HeightOverride( 200.0f )
					[
						SNew( SBorder )
							.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
							.ColorAndOpacity( this, &SScriptEditorConsole::GetAnimatedColorAndOpacity )
							.BorderBackgroundColor( this, &SScriptEditorConsole::GetAnimatedSlateColor )
							[
								SNew( SSpacer )
							]
					]
				]
		]

		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding( 10.0f )
		[
			SNew(SBox)
			.HeightOverride( 26.0f )
			.HAlign( HAlign_Left )
			[
				SNew( SBorder )
				.Padding( FMargin(2) )
				.BorderImage( FEditorStyle::GetBrush( "PythonConsole.Background" ) )
				.ColorAndOpacity( this, &SScriptEditorConsole::GetAnimatedColorAndOpacity )
				.BorderBackgroundColor( this, &SScriptEditorConsole::GetAnimatedSlateColor )
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(3.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("Console", "ConsoleLabel", "Console"))

					]
					+ SHorizontalBox::Slot()
					.Padding(5.0f, 2.0f)
					.VAlign(VAlign_Center)
					.MaxWidth(400.0f)
					[
						SAssignNew(ConsoleInputBox, SScriptConsoleInputBox)
						.OnConsoleCommandExecuted(PythonConsoleDelegates->OnConsoleCommandExecuted)
					]
				]
			]
		]
	];

	EditableTextBox = ConsoleInputBox->GetEditableTextBox();

	// Kick off intro animation
	AnimCurveSequence = FCurveSequence();
	AnimCurve = AnimCurveSequence.AddCurve( 0.0f, ScriptEditorConsoleDefs::IntroAnimationDuration, ECurveEaseFunction::QuadOut );
	FlashCurve = AnimCurveSequence.AddCurve( ScriptEditorConsoleDefs::IntroAnimationDuration, .15f, ECurveEaseFunction::QuadInOut );

	AnimCurveSequence.Play(this->AsShared());
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SScriptEditorConsole::SScriptEditorConsole()
	: CurrentStyle( EScriptEditorConsoleStyle::Compact )
{
}


void SScriptEditorConsole::SetFocusToEditableText()
{
	FSlateApplication::Get().SetKeyboardFocus( EditableTextBox.ToSharedRef(), EFocusCause::SetDirectly );
}

EVisibility SScriptEditorConsole::MakeVisibleIfLogIsShown() const
{
	return CurrentStyle == EScriptEditorConsoleStyle::WithLog ? EVisibility::Visible : EVisibility::Collapsed;
}


FLinearColor SScriptEditorConsole::GetAnimatedColorAndOpacity() const
{
	return FLinearColor( 1.0f, 1.0f, 1.0f, AnimCurve.GetLerp() );
}


FSlateColor SScriptEditorConsole::GetAnimatedSlateColor() const
{
	return FSlateColor( GetAnimatedColorAndOpacity() );
}

FSlateColor SScriptEditorConsole::GetFlashColor() const
{
	float FlashAlpha = 1.0f - FlashCurve.GetLerp();

	if (FlashAlpha == 1.0f)
	{
		FlashAlpha = 0.0f;
	}

	return FLinearColor(1,1,1,FlashAlpha);
}