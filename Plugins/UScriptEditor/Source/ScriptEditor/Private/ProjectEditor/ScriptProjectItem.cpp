// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "ScriptProjectItem.h"
#include "EditorAssetLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/Package.h"
#include "IDirectoryWatcher.h"
#include "DirectoryScanner.h"
#include "DirectoryWatcherModule.h"
#include "ScriptHelperBPFunLib.h"
#include "ScriptEditorSetting.h"

UScriptProjectItem::UScriptProjectItem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LegalExtensions.Add("h");
	LegalExtensions.Add("cpp");
	LegalExtensions.Add("cs");
	LegalExtensions.Add("lua");
	LegalExtensions.Add("js");

	bWithLegalFile = false;
}

void UScriptProjectItem::RescanChildren(bool ShowEmptyFolder)
{
	if(Path.Len() > 0)
	{
		bShowEmptyFolder = ShowEmptyFolder;

		FDirectoryScanner::OnDirectoryScannedOver = FOnDirectoryScannedOver::CreateUObject(this, &UScriptProjectItem::DirectoryScannedOver);
		FDirectoryScanner::AddDirectory(Path, FOnDirectoryScanned::CreateUObject(this, &UScriptProjectItem::HandleDirectoryScanned));
	}
}

void UScriptProjectItem::DeletedEmptyFolder()
{
	if (this->Children.Num() > 0)
	{
		DeleteUnlegalChildren(this);
	}
}

void UScriptProjectItem::HandleDirectoryScanned(const FString& InPathName, EScriptProjectItemType::Type InType)
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
		UScriptProjectItem* NewItem = NewObject<UScriptProjectItem>(GetOutermost(), UScriptProjectItem::StaticClass());
		NewItem->Type = InType;
		NewItem->Path = InPathName;
		NewItem->Name = FPaths::GetCleanFilename(InPathName);
		NewItem->Parent = this;
		if(InType != EScriptProjectItemType::Folder)
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
				//
				for (FString EdtingFile :UScriptEdtiorSetting::Get()->EdittingFiles)
				{
					if (NewItem->Path == EdtingFile)
					{
						UScriptEdtiorSetting::Get()->PreEdittingItems.Add(NewItem);
						break;
					}
				}
				//
			}
		}

		Children.Add(NewItem);
		
		Children.Sort(
			[](const UScriptProjectItem& ItemA, const UScriptProjectItem& ItemB) -> bool
			{
				if(ItemA.Type != ItemB.Type)
				{
					return ItemA.Type < ItemB.Type;
				}

				return ItemA.Name.Compare(ItemB.Name) < 0;
			}
		);

		if(InType == EScriptProjectItemType::Folder)
		{
			// kick off another scan for subdirectories
			FDirectoryScanner::AddDirectory(InPathName, FOnDirectoryScanned::CreateUObject(NewItem, &UScriptProjectItem::HandleDirectoryScanned));

			// @TODO: now register for any changes to this directory if needed
			//FDirectoryWatcherModule& DirectoryWatcherModule = FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher"));
			//DirectoryWatcherModule.Get()->RegisterDirectoryChangedCallback_Handle(InPathName, IDirectoryWatcher::FDirectoryChanged::CreateUObject(NewItem, &UCodeProjectItem::HandleDirectoryChanged), OnDirectoryChangedHandle);
		}
	}
}

void UScriptProjectItem::HandleDirectoryChanged(const TArray<FFileChangeData>& FileChanges)
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

bool UScriptProjectItem::IsLegalFile()const
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

bool UScriptProjectItem::IsEmptyFolder()const
{
	return Type== EScriptProjectItemType::Folder && !bWithLegalFile;
}

UScriptProjectItem* UScriptProjectItem::FindChild(FString ChildFullPath)
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
	
	UScriptProjectItem* OutChild = nullptr;
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


FName UScriptProjectItem::GetBrush() const
{
	switch (Type)
	{
	case EScriptProjectItemType::Project:
		return "ProjectEditor.Icon.Project";
	case EScriptProjectItemType::Folder:
		return "ProjectEditor.Icon.Folder";
	case EScriptProjectItemType::File:
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

void UScriptProjectItem::RescaParentIsLegal(UScriptProjectItem* InParent)
{
	InParent->bWithLegalFile = true;
	if (InParent->Parent)
	{
		RescaParentIsLegal(InParent->Parent);
	}
}

void UScriptProjectItem::DeleteUnlegalChildren(UScriptProjectItem* InParent)
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

void UScriptProjectItem::FindChild(UScriptProjectItem* InParent, FString ChildFullPath, OUT UScriptProjectItem*  &OutChild)
{
	if (InParent && InParent->Children.Num() > 0)
	{
		for (UScriptProjectItem* Child : InParent->Children)
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

bool UScriptProjectItem::BuildScriptAssetContext()
{
	if (Extension == "lua")
	{
		//UnLua.lua do not need to created script asset.
		if (Name == "UnLua.lua")return true;
		//Check file exist?
		FString ScriptContentPath = ScriptEditorUtils::CoverScriptPathToContentPath(Path);
		FString ScriptAssetPath = ScriptEditorUtils::CovertContentPathToAssetPath(ScriptContentPath);

		if (UEditorAssetLibrary::DoesAssetExist(ScriptAssetPath))
		{
			UScriptDataAsset* ScriptAsset = Cast<UScriptDataAsset>(UEditorAssetLibrary::LoadAsset(ScriptAssetPath));
			if (ScriptAsset)
			{
				ScriptDataAsset = ScriptAsset;
				ScriptAsset->Path = ScriptEditorUtils::CoverToRelativeScriptPath(Path);
				//
				TArray<uint8> CodeByte;
				FString CodeText;
				if (FFileHelper::LoadFileToString(CodeText, *Path))
				{
					FFileHelper::LoadFileToArray(CodeByte, *Path);

					ScriptAsset->SourceCode = CodeText;
					ScriptAsset->ByteCode = CodeByte;
					ScriptAsset->UserObject = this;
				}
				//Log
				//GEditor->AddOnScreenDebugMessage(0, 1, FColor::Red, "Build Asset Content:" + ScriptAssetPath);

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

				US_Log("Create A New Asset Content:%s",*ScriptAssetPath);

				return true;
			}
			else
			{
				//LogError
				US_Error_Log("Can not Create Asset Content from:%s", *Path);
				return false;
			}
		}

	}
	return false;
}

void UScriptProjectItem::DirectoryScannedOver()
{
	if (!bShowEmptyFolder)
	{
		this->DeletedEmptyFolder();
	}

	if (OnDirectoryScannedOver.IsBound())
	{
		OnDirectoryScannedOver.ExecuteIfBound();
	}

}
