// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "CodeProjectItem.h"
#include "SourceProject.generated.h"

UCLASS()
class USourceProject : public UCodeProjectItem
{
	GENERATED_UCLASS_BODY()

	// @TODO: This class should probably be mostly config/settings stuff, with a details panel allowing editing somewhere
};
