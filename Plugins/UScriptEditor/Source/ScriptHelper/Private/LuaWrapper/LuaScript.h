#pragma once

#include "ScriptDataAsset.h"
#include "LuaScript.generated.h"

UCLASS(Blueprintable, BlueprintType)
class  SCRIPTHELPER_API ULuaScript : public UScriptDataAsset
{
	GENERATED_BODY()

public:
	ULuaScript(const FObjectInitializer& ObjectInitializer);

};
