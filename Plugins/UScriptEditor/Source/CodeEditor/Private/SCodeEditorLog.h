#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBar;

class SCodeEditorLog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCodeEditorLog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	TSharedPtr<SScrollBar> HorizontalScrollbar;
	TSharedPtr<SScrollBar> VerticalScrollbar;

	TSharedPtr<class SMultiLineEditableText> ScriptLogText;
};