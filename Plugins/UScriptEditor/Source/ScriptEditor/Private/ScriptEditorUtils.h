#pragma once

#include "CoreMinimal.h"
#include "ScriptEditorType.h"

DECLARE_LOG_CATEGORY_EXTERN(UScriptLog, Verbose, All);
#define US_Log(Message,...) UE_LOG(UScriptLog, Log, TEXT(Message), ##__VA_ARGS__)
#define US_Display_Log(Message,...) UE_LOG(UScriptLog, Display, TEXT(Message), ##__VA_ARGS__)
#define US_Warning_Log(Message,...) UE_LOG(UScriptLog, Warning, TEXT(Message), ##__VA_ARGS__)
#define US_Error_Log(Message,...) UE_LOG(UScriptLog, Error, TEXT(Message), ##__VA_ARGS__)

namespace ScriptEditorUtils
{
	/************************************************************************/
	/* Log Helper                                                                      
	/************************************************************************/

	/************************************************************************/
	/*File Helper                                                                  

	/*
	*
	*/

	bool SaveScriptAsset(class UScriptDataAsset* ScriptAsset);
	/*
	*
	*/
	void BrowserToScriptAsset(class UScriptDataAsset* ScriptAsset);

	/*
	*
	*/
	FString CreateLuaFileFromLuaScriptAsset(class ULuaScript* ScriptAsset, EScriptTemplateType TemplateType = EScriptTemplateType::Actor);
	
	class ULuaScript* CreateLuaScriptAssetFromLuaFile(FString LuaScriptFileFullPath);

	FString CreateLuaTemplate(EScriptTemplateType TempType, FString TemplateName);

	//
	bool StringToByteArray(FString InString, TArray<uint8>& InArray);
	//
	FString CoverScriptPathToContentPath(FString ScriptFullPath);

    FString CovertContentPathToGamePath(FString ContentPath);

	FString CovertGamePathToContentPath(FString GamePath);

	FString CovertContentPathToAssetPath(FString ContentFilePath);

	FString CovertAssetPathToContentPath(FString AssetPath);

	FString CovertContentPathToScriptPath(FString ContentFilePath);

	FString CoverToAbsoluteScriptPath(FString RelativeScriptPath);
	FString CoverToRelativeScriptPath(FString AbsoluteScriptPath);
	/************************************************************************/
}
