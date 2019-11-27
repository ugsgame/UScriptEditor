#pragma once

#include "IScriptEditorModule.h"
#include "CoreMinimal.h"

#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"

class FExtender;
class FScriptEditor;

class FToolBarBuilder;
class FMenuBuilder;

class FScriptEditorModule : public IScriptEditorModule
{

public:
	// Get Quest Extension Editor module instance
	FORCEINLINE static FScriptEditorModule* GetInstance() {
		return &FModuleManager::LoadModuleChecked<FScriptEditorModule>("ScriptEditor");
	}

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

	EAssetTypeCategories::Type GetAssetCategoryBit() const { return AssetCategoryBit; };
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	};

	void OnScriptAssetDeleted(UObject*  AssetObject);

	void OnScriptAssetRenamed(const FAssetData& RenamedAsset, const FString& OldObjectPath);

	void OnClearInvalidScriptAssets();
private:
	EAssetTypeCategories::Type AssetCategoryBit;

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	TSharedPtr<class FUICommandList> PluginCommands;

	//TSharedPtr<FCodeProjectEditor> CodeProjectEditorInstance;

public:
	static const FName ScriptEditorTabName;
};