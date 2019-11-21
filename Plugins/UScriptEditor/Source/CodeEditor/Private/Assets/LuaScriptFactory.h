// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LuaScriptFactory.generated.h"

/**
 * 
 */
UCLASS()
class ULuaScriptFactory : public UFactory
{
	GENERATED_BODY()

	ULuaScriptFactory(const FObjectInitializer& ObjectInitializer);

	// UFactory interface
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

	//virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled);

	//virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn)override;

	//virtual bool FactoryCanImport(const FString& Filename) override;
	// End of UFactory interface

// 	virtual uint32 GetMenuCategories() const override;
 	virtual FText GetDisplayName() const override;
	virtual FString GetDefaultNewAssetName() const override;
};
