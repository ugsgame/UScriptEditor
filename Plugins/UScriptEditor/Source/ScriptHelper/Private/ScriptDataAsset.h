#pragma once

#include "Runtime/Engine/Classes/Engine/DataAsset.h"
#include "CoreMinimal.h"
#include "ScriptDataAsset.generated.h"

UCLASS(Blueprintable, BlueprintType)
class  SCRIPTHELPER_API UScriptDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UScriptDataAsset(const FObjectInitializer& ObjectInitializer);

public:

#if WITH_EDITORONLY_DATA
	bool EFlag_IsEditing;
	bool EFlag_IsValid;
#endif // WITH_EDITORONLY_DATA

public:
	UPROPERTY()
	FString Path;
	UPROPERTY()
	FString CodeText;

	//UPROPERTY()
	UObject* UserObject;
};