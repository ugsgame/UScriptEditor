// Fill out your copyright notice in the Description page of Project Settings.


#include "NativeScriptComponent.h"
#include "ScriptDataAsset.h"
#include "ScriptHelperBPFunLib.h"

// Sets default values for this component's properties
UNativeScriptComponent::UNativeScriptComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	bWantsInitializeComponent = true;
	bAutoActivate = true;
	// ...

	HasScriptBinding = false;
}

void UNativeScriptComponent::PostLoad()
{
	Super::PostLoad();

	if(!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

void UNativeScriptComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (!HasScriptBinding)
		HasScriptBinding = UScriptHelperBPFunLib::TryToBindingScript(this, ScriptData);
}

// Called when the game starts
void UNativeScriptComponent::BeginPlay()
{
	Super::BeginPlay();
	// ...
}

UScriptDataAsset* UNativeScriptComponent::GetScriptData()
{
	return ScriptData;
}

// Called every frame
void UNativeScriptComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// ...
}


