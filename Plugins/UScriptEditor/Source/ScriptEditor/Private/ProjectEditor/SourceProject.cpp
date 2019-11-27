// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SourceProject.h"
#include "Misc/Paths.h"


USourceProject::USourceProject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Path = FPaths::GameSourceDir();
}
