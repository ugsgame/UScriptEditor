// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScriptProjectItem.h"

DECLARE_DELEGATE_TwoParams(FOnDirectoryScanned, const FString& /*InPathName*/, EScriptProjectItemType::Type /*InType*/);
DECLARE_DELEGATE(FOnDirectoryScannedOver);

class FDirectoryScanner
{
public:
	static bool Tick();

	static void AddDirectory(const FString& PathName, const FOnDirectoryScanned& OnDirectoryScanned);

	static bool IsScanning() ;

public:
	static TArray<struct FDirectoryScannerCommand*> CommandQueue;
	static FOnDirectoryScannedOver OnDirectoryScannedOver;

	static bool bDataDirty;

private:
	static bool bScanOverFlag;
};
