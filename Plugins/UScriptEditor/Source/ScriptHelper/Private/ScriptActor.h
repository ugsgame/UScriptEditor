// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScriptActor.generated.h"

UCLASS()
class AScriptActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AScriptActor();

protected:
	//For actors statically placed in a level
	virtual void PostLoad() override;
	//When an actor is created in the editor or during Gameplay(runtime spawn actor)
	virtual void PostActorCreated() override;
	//Called after the actor's components have been initialized, only during gameplay and some editor previews.
	virtual void PostInitializeComponents() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Script")
	class UScriptDataAsset* ScriptData;
	
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Script")
	bool HasScriptBinding;
};
