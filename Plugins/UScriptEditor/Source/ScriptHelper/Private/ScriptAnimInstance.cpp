
#include "ScriptAnimInstance.h"

UScriptAnimInstance::UScriptAnimInstance(const FObjectInitializer& ObjectInitializer)
:Super(ObjectInitializer)
{
	HasScriptBinding = false;
}

void UScriptAnimInstance::PostLoad()
{
	Super::PostLoad();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

void UScriptAnimInstance::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

void UScriptAnimInstance::NativeInitializeAnimation()
{
	UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

