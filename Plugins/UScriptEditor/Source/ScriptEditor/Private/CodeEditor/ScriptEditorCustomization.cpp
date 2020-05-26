// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditorCustomization.h"

UScriptEditorCustomization::UScriptEditorCustomization(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

const FScriptEditorControlCustomization& UScriptEditorCustomization::GetControl(const FName& ControlCustomizationName)
{
	static FScriptEditorControlCustomization Default;

	return Default;
}

const FScriptEditorTextCustomization& UScriptEditorCustomization::GetText(const FName& TextCustomizationName)
{
	static FScriptEditorTextCustomization Default;

	return Default;
}
