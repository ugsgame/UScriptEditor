// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "ScriptDataAsset.h"
#include "CodeProjectItem.generated.h"

/** Types of project items. Note that the enum ordering determines the tree sorting */
UENUM()
namespace ECodeProjectItemType
{
	enum Type
	{
		Project,
		Folder,
		File
	};
}

UCLASS()
class UCodeProjectItem : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	void RescanChildren(bool ShowEmptyFolder = false);

	void DeletedEmptyFolder();

	void HandleDirectoryScanned(const FString& InPathName, ECodeProjectItemType::Type InType);

	/** Handle directory changing */
	void HandleDirectoryChanged(const TArray<struct FFileChangeData>& FileChanges);

	bool IsLegalFile()const;

	bool IsEmptyFolder()const;

	UCodeProjectItem* FindChild(FString ChildFullPath);

	FName GetBrush() const;

	DECLARE_DELEGATE(FOnDirectoryScannedOver);
	FOnDirectoryScannedOver OnDirectoryScannedOver;
protected:

	bool BuildScriptAssetContext();

	void DirectoryScannedOver();
private:
	void RescaParentIsLegal(UCodeProjectItem* InParent);

	void DeleteUnlegalChildren(UCodeProjectItem* InParent);

	void FindChild(UCodeProjectItem* InParent, FString ChildFullPath, OUT UCodeProjectItem* & OutChild);
public:
	UPROPERTY(Transient)
	TEnumAsByte<ECodeProjectItemType::Type> Type;

	UPROPERTY(Transient)
	FString Name;

	UPROPERTY(Transient)
	FString Extension;

	UPROPERTY(Transient)
	FString Path;

	UPROPERTY(Transient)
	UCodeProjectItem* Parent;

	UPROPERTY(Transient)
	TArray<UCodeProjectItem*> Children;

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
