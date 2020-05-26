// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptProjectFactory.h"
#include "SourceProject.h"


#define LOCTEXT_NAMESPACE "UScriptEditor"


UScriptProjectFactory::UScriptProjectFactory(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USourceProject::StaticClass();
}


UObject* UScriptProjectFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	USourceProject* NewCodeProject = NewObject<USourceProject>(InParent, Class, Name, Flags);
	return NewCodeProject;
}


#undef LOCTEXT_NAMESPACE
