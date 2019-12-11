#include "ScriptDataAsset.h"
#include "Runtime/Core/Public/Misc/OutputDeviceNull.h"

UScriptDataAsset::UScriptDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	EFlag_IsValid = true;
#endif // WITH_EDITORONLY_DATA
}

FString UScriptDataAsset::GetPath() const
{
	return Path;
}

FString UScriptDataAsset::GetCode() const
{
	return CodeText;
}

