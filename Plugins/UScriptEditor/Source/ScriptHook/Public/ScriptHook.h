#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SCRIPTHOOK_API FScriptHookModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FScriptHookModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FScriptHookModule>("ScriptHook");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("ScriptHook");
	}

protected:


};