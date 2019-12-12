#pragma once

#include "CoreMinimal.h"
#include "ScriptEditorSetting.generated.h"


UCLASS(config = Editor)
class  UScriptEdtiorSetting : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	static UScriptEdtiorSetting* Get();

	UPROPERTY(config)
	TArray<FString> EdittingFiles;

	//For UCodeProjectItem scanning files optimization
	TArray<class UCodeProjectItem*> PreEdittingItems;
};