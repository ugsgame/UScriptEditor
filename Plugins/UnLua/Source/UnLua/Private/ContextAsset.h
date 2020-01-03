﻿#pragma once

#include "Runtime/Engine/Classes/Engine/DataAsset.h"
#include "CoreMinimal.h"
#include "ContextAsset.generated.h"

UCLASS(Blueprintable, BlueprintType)
class  UContextAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UContextAsset(const FObjectInitializer& ObjectInitializer);

public:

#if WITH_EDITORONLY_DATA
	bool EFlag_IsValid;
#endif // WITH_EDITORONLY_DATA

	UFUNCTION(BlueprintCallable, Category = Script)
	FString GetDotPath() const;
	UFUNCTION(BlueprintCallable,Category = Script)
	FString GetPath() const;

	UFUNCTION(BlueprintCallable,Category = Script)
	FString GetSourceCode() const;

	UFUNCTION(BlueprintCallable, Category = Script)
	TArray<uint8> GetByteCode() const;

public:
	UPROPERTY()
	FString Path;
	UPROPERTY()
	FString SourceCode;
	UPROPERTY()
	TArray<uint8>  ByteCode;

	//UPROPERTY()
	UObject* UserObject;
};