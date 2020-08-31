#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScriptEditorBPFunLib.generated.h"

UCLASS(Blueprintable, BlueprintType)
class SCRIPTEDITOR_API UScriptEdtiorBPFunLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION()
	static bool CreateScriptAssetFromFileAndEdited(FString ScriptFileFullPath);
};