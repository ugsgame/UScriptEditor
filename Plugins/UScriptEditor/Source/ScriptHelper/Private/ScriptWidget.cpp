
#include "ScriptWidget.h"
#include "ScriptHelperBPFunLib.h"

UScriptWidget::UScriptWidget(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	HasScriptBinding = false;
}

void UScriptWidget::PostLoad()
{
	Super::PostLoad();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

bool UScriptWidget::Initialize()
{
	bool rel = Super::Initialize();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);

	return rel;
}
