// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CodeProjectItem.h"
#include "Misc/Paths.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/Package.h"
#include "IDirectoryWatcher.h"
#include "DirectoryScanner.h"
#include "DirectoryWatcherModule.h"

UCodeProjectItem::UCodeProjectItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LegalExtensions.Add("h");
	LegalExtensions.Add("cpp");
	LegalExtensions.Add("cs");
	LegalExtensions.Add("lua");
	LegalExtensions.Add("js");

	bWithLegalFile = false;
}

void UCodeProjectItem::RescanChildren(bool ShowEmptyFolder)
{
	if(Path.Len() > 0)
	{
		if (!ShowEmptyFolder)
		{
			FDirectoryScanner::OnDirectoryScannedOver = FOnDirectoryScannedOver::CreateUObject(this, &UCodeProjectItem::DeletedEmptyFolder);
		}

		FDirectoryScanner::AddDirectory(Path, FOnDirectoryScanned::CreateUObject(this, &UCodeProjectItem::HandleDirectoryScanned));
	}
}

void UCodeProjectItem::DeletedEmptyFolder()
{
	if (this->Children.Num() > 0)
	{
		DeleteUnlegalChildren(this);
	}
}

void UCodeProjectItem::HandleDirectoryScanned(const FString& InPathName, ECodeProjectItemType::Type InType)
{
	// check for a child that already exists
	bool bCreateNew = true;
	for(const auto& Child : Children)
	{
		if(Child->Type == InType && Child->Path == InPathName)
		{
			bCreateNew = false;
			break;
		}
	}

	// create children now & kick off their scan
	if(bCreateNew)
	{
		UCodeProjectItem* NewItem = NewObject<UCodeProjectItem>(GetOutermost(), UCodeProjectItem::StaticClass());
		NewItem->Type = InType;
		NewItem->Path = InPathName;
		NewItem->Name = FPaths::GetCleanFilename(InPathName);
		NewItem->Parent = this;
		if(InType != ECodeProjectItemType::Folder)
		{
			NewItem->Extension = FPaths::GetExtension(InPathName);
			//Check Extension!!
			if (!NewItem->IsLegalFile())
			{
				return;
			}
			else
			{
				NewItem->bWithLegalFile = true;
				//Have an legal file,tell my parent it is legal too!! 
 				this->RescaParentIsLegal(this);
			}
		}

		Children.Add(NewItem);
		
		Children.Sort(
			[](const UCodeProjectItem& ItemA, const UCodeProjectItem& ItemB) -> bool
			{
				if(ItemA.Type != ItemB.Type)
				{
					return ItemA.Type < ItemB.Type;
				}

				return ItemA.Name.Compare(ItemB.Name) < 0;
			}
		);

		if(InType == ECodeProjectItemType::Folder)
		{
			// kick off another scan for subdirectories
			FDirectoryScanner::AddDirectory(InPathName, FOnDirectoryScanned::CreateUObject(NewItem, &UCodeProjectItem::HandleDirectoryScanned));

			// @TODO: now register for any changes to this directory if needed
			//FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
			//DirectoryWatcherModule.Get()->RegisterDirectoryChangedCallback_Handle(InPathName, IDirectoryWatcher::FDirectoryChanged::CreateUObject(NewItem, &UCodeProjectItem::HandleDirectoryChanged), OnDirectoryChangedHandle);
		}
	}
}

void UCodeProjectItem::HandleDirectoryChanged(const TArray<FFileChangeData>& FileChanges)
{
	// @TODO: dynamical update directory watchers so we can update the view in real-time
	for(const auto& Change : FileChanges)
	{
		switch(Change.Action)
		{
		default:
		case FFileChangeData::FCA_Unknown:
			break;
		case FFileChangeData::FCA_Added:
			{

			}
			break;
		case FFileChangeData::FCA_Modified:
			{

			}
			break;
		case FFileChangeData::FCA_Removed:
			{

			}
			break;
		}
	}
}

bool UCodeProjectItem::IsLegalFile()const
{
	for (FString CompareEx:LegalExtensions)
	{
		if (CompareEx == Extension)
		{
			return true;
		}
	}
	return false;
}

bool UCodeProjectItem::IsEmptyFolder()const
{
	return Type== ECodeProjectItemType::Folder && !bWithLegalFile;
}

void UCodeProjectItem::RescaParentIsLegal(UCodeProjectItem* InParent)
{
	InParent->bWithLegalFile = true;
	if (InParent->Parent)
	{
		RescaParentIsLegal(InParent->Parent);
	}
}

void UCodeProjectItem::DeleteUnlegalChildren(UCodeProjectItem* InParent)
{
	if (InParent->bWithLegalFile)
	{
		if (InParent->Children.Num()>0)
		{
			for (int32 i=0;i< InParent->Children.Num();i++)
			{
				if (!InParent->Children[i]->bWithLegalFile)
				{
					InParent->Children.RemoveAt(i);
					i--;
				}
				else
				{
					this->DeleteUnlegalChildren(InParent->Children[i]);
				}
			}
		}
		else
		{
			InParent->Children.Empty();
		}
	}
	else
	{
		InParent->Children.Empty();
	}
}