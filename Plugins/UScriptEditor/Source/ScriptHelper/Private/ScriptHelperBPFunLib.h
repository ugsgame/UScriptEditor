// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ScriptHelperBPFunLib.generated.h"
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SCRIPTHELPER_API UScriptHelperBPFunLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "UBPFuncLib")
	static FString ScriptSourceRoot();

	UFUNCTION(BlueprintPure, Category = "UBPFuncLib")
	static FString ScriptSourceDir();
};
