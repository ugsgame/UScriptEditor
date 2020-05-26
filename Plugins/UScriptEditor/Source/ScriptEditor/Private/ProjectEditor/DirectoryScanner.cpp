// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "DirectoryScanner.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/IQueuedWork.h"
#include "Misc/QueuedThreadPool.h"

TArray<FDirectoryScannerCommand*> FDirectoryScanner::CommandQueue;
bool FDirectoryScanner::bScanOverFlag = false;
FOnDirectoryScannedOver FDirectoryScanner::OnDirectoryScannedOver;

struct FDirectoryResult
{
	FDirectoryResult(const FString& InPathName, EScriptProjectItemType::Type InType)
		: PathName(InPathName)
		, Type(InType)
	{
	}

	FString PathName;

	EScriptProjectItemType::Type Type;
};

struct FDirectoryScannerCommand : public IQueuedWork
{
	FDirectoryScannerCommand(const FString& InPathName, const FOnDirectoryScanned& InOnDirectoryScanned)
		: PathName(InPathName)
		, OnDirectoryScanned(InOnDirectoryScanned)
		, bExecuted(0)
	{
	}

	/** Begin FQueuedWork interface */
	virtual void Abandon() override
	{
		FPlatformAtomics::InterlockedExchange(&bExecuted, 1);
	}

	virtual void DoThreadedWork() override
	{
		class FDirectoryEnumerator : public IPlatformFile::FDirectoryVisitor
		{
		public:
			FDirectoryEnumerator(TLockFreePointerListUnordered<FDirectoryResult, PLATFORM_CACHE_LINE_SIZE>& InFoundFiles)
				: FoundFiles(InFoundFiles)
			{
			}

			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
			{
				if(bIsDirectory)
				{
					FoundFiles.Push(new FDirectoryResult(FilenameOrDirectory, EScriptProjectItemType::Folder));
				}
				else
				{
					FoundFiles.Push(new FDirectoryResult(FilenameOrDirectory, EScriptProjectItemType::File));
				}

				return true;
			}

			TLockFreePointerListUnordered<FDirectoryResult, PLATFORM_CACHE_LINE_SIZE>& FoundFiles;
		};

		FDirectoryEnumerator DirectoryEnumerator(FoundFiles);
		IPlatformFile& PlatformFile = IPlatformFile::GetPlatformPhysical();
		PlatformFile.IterateDirectory(*PathName, DirectoryEnumerator);

		FPlatformAtomics::InterlockedExchange(&bExecuted, 1);
	}
	/** End FQueuedWork interface */

	FString PathName;

	FOnDirectoryScanned OnDirectoryScanned;

	TLockFreePointerListUnordered<FDirectoryResult, PLATFORM_CACHE_LINE_SIZE> FoundFiles;

	volatile int32 bExecuted;
};


bool FDirectoryScanner::Tick()
{
	bool bAddedData = false;
	for (int32 CommandIndex = 0; CommandIndex < CommandQueue.Num(); ++CommandIndex)
	{
		FDirectoryScannerCommand& Command = *CommandQueue[CommandIndex];
		if (Command.bExecuted)
		{
			while(!Command.FoundFiles.IsEmpty())
			{
				FDirectoryResult* DirectoryResult = Command.FoundFiles.Pop();
				Command.OnDirectoryScanned.ExecuteIfBound(DirectoryResult->PathName, DirectoryResult->Type);
				delete DirectoryResult;
				bAddedData = true;
			}

			// Remove command from the queue & delete it, we are done for this tick
			CommandQueue.RemoveAt(CommandIndex);
			delete &Command;
			break;
		}
	}

	if (!IsScanning())
	{
		if (bScanOverFlag && OnDirectoryScannedOver.IsBound())
		{
			OnDirectoryScannedOver.ExecuteIfBound();
			OnDirectoryScannedOver.Unbind();
			bScanOverFlag = false;
		}
	}

	return bAddedData;
}

void FDirectoryScanner::AddDirectory(const FString& PathName, const FOnDirectoryScanned& OnDirectoryScanned)
{
	FDirectoryScannerCommand* NewCommand = new FDirectoryScannerCommand(PathName, OnDirectoryScanned);
	CommandQueue.Add(NewCommand);
	GThreadPool->AddQueuedWork(NewCommand);

	bScanOverFlag = true;
}

bool FDirectoryScanner::IsScanning()
{
	return CommandQueue.Num() > 0;
}
