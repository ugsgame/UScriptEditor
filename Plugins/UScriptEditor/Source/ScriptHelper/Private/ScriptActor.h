// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
//#include "UnLuaInterface.h"
#include "ScriptActor.generated.h"

UCLASS()
class SCRIPTHELPER_API AScriptActor : public AActor/*,public IUnLuaInterface*/
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
	//TODO:Using IUnLuaInterface in CPP would crash in OnAsyncLoadingFlushUpdate
	//UnLua Module Interface
	//virtual FString GetModuleName_Implementation() const override;
	//virtual void GetModuleContext_Implementation(FString& Path, FString& SourceCode, TArray<uint8>& ByteCode) const override;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, /*EditDefaultsOnly,*/ Category = "Script")
	class UScriptDataAsset* ScriptData;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Transient, Category = "Script")
	bool HasScriptBinding;
};
