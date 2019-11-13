#pragma once

#include "Modules/ModuleManager.h"

#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"

class FExtender;
class FCodeProjectEditor;

class FCodeEditor : public IModuleInterface
{

public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Get Quest Extension Editor module instance
	FORCEINLINE static FCodeEditor* GetInstance() {
		return &FModuleManager::LoadModuleChecked<FCodeEditor>("CodeEditor");
	}

private:
	EAssetTypeCategories::Type AssetCategoryBit;
	TSharedPtr<FExtender> Extender;


	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;
public:
	EAssetTypeCategories::Type GetAssetCategoryBit() const { return AssetCategoryBit; };

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	};
};