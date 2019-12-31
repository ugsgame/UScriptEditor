﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NativeScriptComponent.generated.h"


UCLASS(Blueprintable, BlueprintType,ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UNativeScriptComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UNativeScriptComponent();
	//
	class UScriptDataAsset* GetScriptData();
protected:

	virtual void PostLoad() override;

	virtual void InitializeComponent() override;

	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category = "Script")
	class UScriptDataAsset* ScriptData;

	UPROPERTY(BlueprintReadOnly,Transient, Category = "Script")
	bool HasScriptBinding;
};
