#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

class SScrollBar;

class SScriptEditorLog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SScriptEditorLog) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

protected:
	TSharedPtr<SScrollBar> HorizontalScrollbar;
	TSharedPtr<SScrollBar> VerticalScrollbar;

	TSharedPtr<class SMultiLineEditableText> ScriptLogText;
};