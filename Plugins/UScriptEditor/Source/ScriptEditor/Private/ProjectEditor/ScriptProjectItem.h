// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "ScriptDataAsset.h"
#include "ScriptProjectItem.generated.h"

/** Types of project items. Note that the enum ordering determines the tree sorting */
UENUM()
namespace EScriptProjectItemType
{
	enum Type
	{
		Project,
		Folder,
		File
	};
}

UCLASS()
class UScriptProjectItem : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	void RescanChildren(bool ShowEmptyFolder = false);

	void DeletedEmptyFolder();

	void HandleDirectoryScanned(const FString& InPathName, EScriptProjectItemType::Type InType);

	/** Handle directory changing */
	void HandleDirectoryChanged(const TArray<struct FFileChangeData>& FileChanges);

	bool IsLegalFile()const;

	bool IsEmptyFolder()const;

	UScriptProjectItem* FindChild(FString ChildFullPath);

	FName GetBrush() const;

	DECLARE_DELEGATE(FOnDirectoryScannedOver);
	FOnDirectoryScannedOver OnDirectoryScannedOver;
protected:

	bool BuildScriptAssetContext();

	void DirectoryScannedOver();
private:
	void RescaParentIsLegal(UScriptProjectItem* InParent);

	void DeleteUnlegalChildren(UScriptProjectItem* InParent);

	void FindChild(UScriptProjectItem* InParent, FString ChildFullPath, OUT UScriptProjectItem* & OutChild);
public:
	UPROPERTY(Transient)
	TEnumAsByte<EScriptProjectItemType::Type> Type;

	UPROPERTY(Transient)
	FString Name;

	UPROPERTY(Transient)
	FString Extension;

	UPROPERTY(Transient)
	FString Path;

	UPROPERTY(Transient)
	UScriptProjectItem* Parent;

	UPROPERTY(Transient)
	TArray<UScriptProjectItem*> Children;

	UPROPERTY(Transient)
	UScriptDataAsset* ScriptDataAsset;

	/** Delegate handle for directory watcher */
	FDelegateHandle OnDirectoryChangedHandle;

	bool bWithLegalFile;

	bool bShowEmptyFolder;
private:
	UPROPERTY(Transient)
	TArray<FString> LegalExtensions;
};
