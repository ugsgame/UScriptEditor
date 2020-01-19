#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EScriptTemplateType : uint8
{
	ActorComponent = 0,
	Actor = 1,
	AnimInstance = 2,
	UserWidget = 3,
};

struct FScriptReferenceInfo
{
	FName ReferencedAsset;			//Only Blueprint or Native Object

	TArray<UBlueprint*> BlueprintClasses;
	TArray<UClass*> NativeClasses;
};

enum class ECompleteParseType : uint8
{
	None = 0,
	Dot = 1,		//.	
	Colon = 2,		//:
};