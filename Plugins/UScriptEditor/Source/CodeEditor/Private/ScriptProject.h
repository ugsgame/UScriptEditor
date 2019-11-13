#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CodeProjectItem.h"
#include "ScriptProject.generated.h"

UCLASS()
class UScriptProject : public UCodeProjectItem
{
	GENERATED_UCLASS_BODY()

		// @TODO: This class should probably be mostly config/settings stuff, with a details panel allowing editing somewhere
};