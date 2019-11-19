#pragma once

DECLARE_LOG_CATEGORY_EXTERN(UScriptEditor, Log, All);

namespace CodeEditorUtils
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
	FString CreateLuaFileFromLuaScriptAsset(class ULuaScript* ScriptAsset);
	
	class ULuaScript* CreateLuaScriptAssetFromLuaFile(FString LuaScriptFilePath);

    FString CovertContentPathToGamePath(FString ContentPath);

	FString CovertContentFilePathToAssetPath(FString ContentFilePath);
}
