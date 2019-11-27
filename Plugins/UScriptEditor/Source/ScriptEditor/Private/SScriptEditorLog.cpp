
#include "SScriptEditorLog.h"
#include "Misc/FileHelper.h"
#include "Framework/Text/TextLayout.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "ScriptEditorStyle.h"
#include "CodeProjectItem.h"
#include "CPPRichTextSyntaxHighlighterTextLayoutMarshaller.h"

#define LOCTEXT_NAMESPACE "ScriptEditorLog"

void SScriptEditorLog::Construct(const FArguments& InArgs)
{
	//TODO:EditorLog

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

	ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FScriptEditorStyle::Get().GetBrush("TextEditor.Border"))
			[
			SNew(SGridPanel)
			.FillColumn(0, 1.0f)
			.FillRow(0, 1.0f)
			+ SGridPanel::Slot(0, 0)
			[
				SAssignNew(ScriptLogText, SMultiLineEditableText)
				.Text(FText::FromString("TODO:This is log!!!"))
				.IsReadOnly(true)
				.Marshaller(RichTextMarshaller)
				.HScrollBar(HorizontalScrollbar)
				.VScrollBar(VerticalScrollbar)
			]
			+ SGridPanel::Slot(1, 0)
			[
				VerticalScrollbar.ToSharedRef()
			]
			+ SGridPanel::Slot(0, 1)
			[
				HorizontalScrollbar.ToSharedRef()
			]
		]
	];
}

#undef LOCTEXT_NAMESPACE