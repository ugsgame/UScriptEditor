// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "ScriptEditorCustomization.generated.h"

USTRUCT()
struct FScriptEditorTextCustomization
{
	GENERATED_USTRUCT_BODY()

	FScriptEditorTextCustomization()
		: Font("")
		, Color(0.0f, 0.0f, 0.0f, 1.0f)
	{
	}

	UPROPERTY(EditAnywhere, Category=Text)
	FString Font;

	UPROPERTY(EditAnywhere, Category=Text)
	FLinearColor Color;
};

USTRUCT()
struct FScriptEditorControlCustomization
{
	GENERATED_USTRUCT_BODY()

	FScriptEditorControlCustomization()
		: Color(0.0f, 0.0f, 0.0f, 1.0f)
	{
	}

	UPROPERTY(EditAnywhere, Category=Controls)
	FLinearColor Color;
};

UCLASS(Config=Editor)
class UScriptEditorCustomization : public UObject
{
	GENERATED_UCLASS_BODY()

	static const FScriptEditorControlCustomization& GetControl(const FName& ControlCustomizationName);

	static const FScriptEditorTextCustomization& GetText(const FName& TextCustomizationName);

private:
	UPROPERTY(EditAnywhere, EditFixedSize, Category=Controls)
	TArray<FScriptEditorControlCustomization> Controls;

	UPROPERTY(EditAnywhere, EditFixedSize, Category=Text)
	TArray<FScriptEditorTextCustomization> Text;
};
