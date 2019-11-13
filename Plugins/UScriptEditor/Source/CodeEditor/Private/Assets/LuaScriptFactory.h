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
	// End of UFactory interface

// 	virtual uint32 GetMenuCategories() const override;
 	virtual FText GetDisplayName() const override;
	virtual FString GetDefaultNewAssetName() const override;
};
