#pragma once

#include "ScriptEditorType.h"

DECLARE_LOG_CATEGORY_EXTERN(UScriptEditor, Log, All);

namespace ScriptEditorUtils
{
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
	
	class ULuaScript* CreateLuaScriptAssetFromLuaFile(FString LuaScriptFilePath);

	FString CreateLuaTemplate(EScriptTemplateType TempType, FString TemplateName);

    FString CovertContentPathToGamePath(FString ContentPath);

	FString CovertGamePathToContentPath(FString GamePath);

	FString CovertContentPathToAssetPath(FString ContentFilePath);

	FString CovertAssetPathToContentPath(FString AssetPath);
}
