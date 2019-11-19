
#include "CodeEditorUtils.h"
#include "UnrealEd.h"
#include "AssetToolsModule.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Assets/LuaScriptFactory.h"
#include "LuaWrapper/LuaScript.h"

DEFINE_LOG_CATEGORY(UScriptEditor);

namespace CodeEditorUtils
{


	bool SaveScriptAsset(UScriptDataAsset* ScriptAsset)
	{
		TArray<UPackage*> Packages;
		Packages.Add(ScriptAsset->GetOutermost()); // Fully load and check out is done in UEditorLoadingAndSavingUtils::SavePackages
		return UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
	}

	void BrowserToScriptAsset(UScriptDataAsset* ScriptAsset)
	{
		TArray<UObject*> ObjectsToSync;
		ObjectsToSync.Add(ScriptAsset);
		GEditor->SyncBrowserToObjects(ObjectsToSync);
	}


	FString CreateLuaFileFromLuaScriptAsset(class ULuaScript* ScriptAsset)
	{
		FString FullPath = ScriptAsset->GetPathName();
		FullPath.RemoveFromStart("/Game");
		FullPath.RemoveFromEnd(ScriptAsset->GetName());
		FullPath = FPaths::ProjectContentDir() + FullPath + "lua";

		FString CodeText;
		if (FFileHelper::LoadFileToString(CodeText, *FullPath))
		{
			ScriptAsset->CodeText = CodeText;
		}
		else
		{
			FFileHelper::SaveStringToFile("", *FullPath);
		}
		ScriptAsset->Path = FullPath;

		return FullPath;
	}

	ULuaScript* CreateLuaScriptAssetFromLuaFile(FString LuaScriptFilePath)
	{
		FString CodeText;
		if (FFileHelper::LoadFileToString(CodeText, *LuaScriptFilePath))
		{
			FString AssetName = FPaths::GetBaseFilename(LuaScriptFilePath);
			FString AssetPath = FPaths::GetPath(CovertContentPathToGamePath(LuaScriptFilePath));

			ULuaScriptFactory* NewFactory = NewObject<ULuaScriptFactory>();

			FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
			ULuaScript* NewAsset = Cast<ULuaScript>(AssetToolsModule.Get().CreateAsset(AssetName, AssetPath, NewFactory->GetSupportedClass(), NewFactory,"CreateFromLuaFile"));

			if (NewAsset)
			{
				NewAsset->Path = LuaScriptFilePath;
				NewAsset->CodeText = CodeText;

				//TArray<UObject*> ObjectsToSync;
				//ObjectsToSync.Add(NewAsset);
				//GEditor->SyncBrowserToObjects(ObjectsToSync);
			}

			return NewAsset;
		}

		return nullptr;
	}

	FString CovertContentPathToGamePath(FString ContentPath)
	{
		if (ContentPath.Contains("/Content"))
		{
			int32 FirstIndex = ContentPath.Find("/Content");
			ContentPath.RemoveAt(0, FirstIndex + 8);
			ContentPath = "/Game" + ContentPath;
		}
		return ContentPath;
	}

	FString CovertContentFilePathToAssetPath(FString ContentFilePath)
	{
		FString AssetFilePath = CovertContentPathToGamePath(FPaths::GetPath(ContentFilePath));
		AssetFilePath = AssetFilePath + "/" + FPaths::GetBaseFilename(ContentFilePath);
		return AssetFilePath;
	}

}
