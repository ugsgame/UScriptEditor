// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CodeProjectFactory.h"
#include "SourceProject.h"


#define LOCTEXT_NAMESPACE "UScriptEditor"


UCodeProjectFactory::UCodeProjectFactory(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USourceProject::StaticClass();
}


UObject* UCodeProjectFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	USourceProject* NewCodeProject = NewObject<USourceProject>(InParent, Class, Name, Flags);
	return NewCodeProject;
}


#undef LOCTEXT_NAMESPACE
