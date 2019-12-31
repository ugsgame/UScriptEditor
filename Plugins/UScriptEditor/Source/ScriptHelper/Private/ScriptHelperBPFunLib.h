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

	UFUNCTION(BlueprintCallable, Category = "Script|UBPFuncLib|Test")
	static bool GetFloatByName(UObject* Target, FName VarName, float &outFloat);

	UFUNCTION(BlueprintCallable, Category = "Script|UBPFuncLib")
	static bool GetScriptDataByName(UObject* Target, FName VarName, class UScriptDataAsset* &outScriptData);

	UFUNCTION(BlueprintPure, Category = "Script|UBPFuncLib")
	static FString ScriptSourceRoot();

	UFUNCTION(BlueprintPure, Category = "Script|UBPFuncLib")
	static FString ScriptSourceDir();

	UFUNCTION(BlueprintCallable, Category = "Script|UBPFuncLib")
	static bool TryToRegisterScriptAsset(FString ModuleName);
	UFUNCTION(BlueprintCallable, Category = "Script|UBPFuncLib")
	static bool TryToBindingScript(UObject* InObject, class UScriptDataAsset *InScriptData);

	//TODO:move to utils category
	static FString ConvertAnyPathToObjectPath(const FString& AnyAssetPath, FString& OutFailureReason);
	static bool HasValidRoot(const FString& ObjectPath);
	static FString RemoveFullName(const FString& AnyAssetPath, FString& OutFailureReason);
	static bool IsAValidPath(const FString& Path, const TCHAR* InvalidChar, FString& OutFailureReason);
	//
};
