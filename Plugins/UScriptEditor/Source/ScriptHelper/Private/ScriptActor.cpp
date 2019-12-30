// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptActor.h"
#include "ScriptHelperBPFunLib.h"

// Sets default values
AScriptActor::AScriptActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	HasScriptBinding = false;
}

void AScriptActor::PostLoad()
{
	Super::PostLoad();

	if(!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

void AScriptActor::PostActorCreated()
{
	Super::PostActorCreated();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

void AScriptActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

// Called when the game starts or when spawned
void AScriptActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AScriptActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
/*
FString AScriptActor::GetModuleName_Implementation() const
{
	if (ScriptData)
	{
		return ScriptData->GetDotPath();
	}
	return TEXT("");
}

void AScriptActor::GetModuleContext_Implementation(FString& Path, FString& SourceCode, TArray<uint8>& ByteCode) const
{
	if (ScriptData)
	{
		Path = ScriptData->GetPath();
		SourceCode = ScriptData->GetSourceCode();
		ByteCode = ScriptData->GetByteCode();
	}
}
*/

