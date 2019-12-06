// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CodeProjectItem.h"
#include "EditorAssetLibrary.h"
#include "Misc/FileHelper.h"
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
				NewItem->BuildScriptAssetContext();
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

UCodeProjectItem* UCodeProjectItem::FindChild(FString ChildFullPath)
{
	//层序遍历
	/*
	TArray<UCodeProjectItem*> Quece;
	UCodeProjectItem* CurrentNode = this;
	while (CurrentNode)
	{
		if (CurrentNode->Path == ChildFullPath)
		{
			return CurrentNode;
		}

		for (UCodeProjectItem* Child: Children)
		{
			Quece.Add(Child);
		}
		if (Quece.Num() == 0)
		{
			return nullptr;
		}
		CurrentNode = Quece[0];
		Quece.RemoveAt(0);
	}
	return nullptr;
	*/
	
	UCodeProjectItem* OutChild = nullptr;
	if (this->Path == ChildFullPath)
	{
		return this;
	}
	else
	{
		FindChild(this, ChildFullPath, OutChild);

	}
	return OutChild;
	
}


FName UCodeProjectItem::GetBrush() const
{
	switch (Type)
	{
	case ECodeProjectItemType::Project:
		return "ProjectEditor.Icon.Project";
	case ECodeProjectItemType::Folder:
		return "ProjectEditor.Icon.Folder";
	case ECodeProjectItemType::File:
	{
		if (Extension == "lua")
		{
			return "ProjectEditor.Icon.lua";
		}
		return "ProjectEditor.Icon.GenericFile";
	}
	default:
		return "ProjectEditor.Icon.GenericFile";
	}
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

void UCodeProjectItem::FindChild(UCodeProjectItem* InParent, FString ChildFullPath, OUT UCodeProjectItem*  &OutChild)
{
	if (InParent && InParent->Children.Num() > 0)
	{
		for (UCodeProjectItem* Child : InParent->Children)
		{
			if (Child->Path == ChildFullPath)
			{
				OutChild = Child;
				return;
			}
			else
			{
				FindChild(Child, ChildFullPath, OutChild);
			}
		}
	}
}

bool UCodeProjectItem::BuildScriptAssetContext()
{
	if (Extension == "lua")
	{
		//Check file exist?
		FString ScriptAssetPath = ScriptEditorUtils::CovertContentPathToAssetPath(Path);

		if (UEditorAssetLibrary::DoesAssetExist(ScriptAssetPath))
		{
			UScriptDataAsset* ScriptAsset = Cast<UScriptDataAsset>(UEditorAssetLibrary::LoadAsset(ScriptAssetPath));
			if (ScriptAsset)
			{
				ScriptDataAsset = ScriptAsset;
				ScriptAsset->Path = Path;
				//
				FString CodeText;
				if (FFileHelper::LoadFileToString(CodeText, *Path))
				{
					ScriptAsset->CodeText = CodeText;
					ScriptAsset->UserObject = this;
				}
				GEditor->AddOnScreenDebugMessage(0, 1, FColor::Red, "Build Asset Content:" + ScriptAssetPath);

				return true;
			}
			else
			{
				//LogError: 
			}
		}
		else
		{
			UScriptDataAsset* ScriptAsset = ScriptEditorUtils::CreateLuaScriptAssetFromLuaFile(Path);
			if (ScriptAsset)
			{
				ScriptDataAsset = ScriptAsset;
				ScriptAsset->UserObject = this;
				GEditor->AddOnScreenDebugMessage(0, 1, FColor::Red, "Create A New Asset Content:" + ScriptAssetPath);

				return true;
			}
			else
			{
				//LogError
			}
		}

	}
	return false;
}