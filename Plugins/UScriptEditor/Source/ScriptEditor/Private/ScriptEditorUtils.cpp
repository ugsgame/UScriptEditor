
#include "ScriptEditorUtils.h"
#include "UnrealEd.h"
#include "AssetToolsModule.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Assets/LuaScriptFactory.h"

#include "ScriptDataAsset.h"
#include "LuaWrapper/LuaScript.h"

#include "IPluginManager.h"

//DEFINE_LOG_CATEGORY(UScriptEditor);

namespace ScriptEditorUtils
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


	FString CreateLuaFileFromLuaScriptAsset(class ULuaScript* ScriptAsset ,EScriptTemplateType TemplateType)
	{
		FString FullPath = ScriptAsset->GetPathName();
		FullPath.RemoveFromStart(TEXT("/Game"));
		FullPath.RemoveFromEnd(ScriptAsset->GetName());
		FullPath = FPaths::ProjectContentDir() + FullPath + TEXT("lua");

		FString CodeText;
		if (FFileHelper::LoadFileToString(CodeText, *FullPath))
		{
			ScriptAsset->CodeText = CodeText;
		}
		else
		{
			FString LuaTemp = CreateLuaTemplate(TemplateType, ScriptAsset->GetName());
			FFileHelper::SaveStringToFile(*LuaTemp, *FullPath);
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
			ULuaScript* NewAsset = Cast<ULuaScript>(AssetToolsModule.Get().CreateAsset(AssetName, AssetPath, NewFactory->GetSupportedClass(), NewFactory, "CreateFromLuaFile"));

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

	FString CreateLuaTemplate(EScriptTemplateType TempType, FString TemplateName)
	{
		FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("UnLua"))->GetContentDir();
		FString TemplatePath;

		switch (TempType)
		{
		case EScriptTemplateType::ActorComponent:
			TemplatePath = ContentDir + TEXT("/ActorComponentTemplate.lua");
		case EScriptTemplateType::Actor:
			TemplatePath = ContentDir + TEXT("/ActorTemplate.lua");
		case  EScriptTemplateType::AnimInstance:
			TemplatePath = ContentDir + TEXT("/AnimInstanceTemplate.lua");
		case EScriptTemplateType::UserWidget:
			TemplatePath = ContentDir + TEXT("/UserWidgetTemplate.lua");
		default:
			TemplatePath = ContentDir + TEXT("/ActorTemplate.lua");
			break;
		}
		FString Content;
		FFileHelper::LoadFileToString(Content, *TemplatePath);
		return Content.Replace(TEXT("TemplateName"), *TemplateName);
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

	FString CovertGamePathToContentPath(FString GamePath)
	{
		if (GamePath.Contains("/Game"))
		{
			int32 FirstIndex = GamePath.Find("/Game/");
			GamePath.RemoveAt(0, FirstIndex + 6);
			GamePath = FPaths::ProjectContentDir() + GamePath;
		}
		return GamePath;
	}

	FString CovertContentPathToAssetPath(FString ContentFilePath)
	{
		FString AssetFilePath = CovertContentPathToGamePath(FPaths::GetPath(ContentFilePath));
		AssetFilePath = AssetFilePath + "/" + FPaths::GetBaseFilename(ContentFilePath);
		return AssetFilePath;
	}

	FString CovertAssetPathToContentPath(FString AssetPath)
	{
		FString ContentFilePath = CovertGamePathToContentPath(FPaths::GetPath(AssetPath));
		ContentFilePath = ContentFilePath + "/" + FPaths::GetBaseFilename(AssetPath);
		return ContentFilePath;
	}

}
