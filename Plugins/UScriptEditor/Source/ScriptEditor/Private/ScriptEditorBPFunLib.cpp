

#include "ScriptEditorBPFunLib.h"

#include "../../ScriptHelper/Private/ScriptDataAsset.h"

bool UScriptEdtiorBPFunLib::CreateScriptAssetFromFileAndEdited(FString ScriptFileFullPath)
{
	UScriptDataAsset* ScriptAsset = ScriptEditorUtils::CreateLuaScriptAssetFromLuaFile(ScriptFileFullPath);
	if (ScriptAsset)
	{
		//
		//
		return true;
	}

	return false;
}
