// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FCodeEditorStyle
{
public:

	static void Initialize();
	static void Shutdown();

	/** reloads textures used by slate renderer */
	static void ReloadTextures();

	static const ISlateStyle& Get();
	static  FName GetStyleSetName();

private:

	/** Singleton instances of this style. */
	static TSharedPtr< class FSlateStyleSet > StyleSet;	
};
