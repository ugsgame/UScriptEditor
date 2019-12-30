#include "ScriptHelperModule.h"
#include "ScriptHelperPrivatePCH.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "Runtime/SlateCore/Public/Rendering/SlateRenderer.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Modules/ModuleManager.h"
#include "ScriptHelperBPFunLib.h"
#include "Paths.h"
#include "FileHelper.h"

#define LOCTEXT_NAMESPACE "ScriptHelper"

class FScriptHelperModule : public IScriptHelperModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	bool SpawnSystemScriptFiles();
	void Initialize(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow);
	void AddGraphicsSwitcher(FToolBarBuilder& ToolBarBuilder);
	TSharedPtr< FExtender > NotificationBarExtender;
	bool bAllowAutomaticGraphicsSwitching;
	bool bAllowMultiGPUs;
};

IMPLEMENT_MODULE(FScriptHelperModule, ScriptHelper)

void FScriptHelperModule::StartupModule()
{
	//Spawn Script
	SpawnSystemScriptFiles();
}

bool FScriptHelperModule::SpawnSystemScriptFiles()
{
	FString ScriptCode;
	FString ScriptPath = UScriptHelperBPFunLib::ScriptSourceDir() + "UnLua.lua";

	//UnLua.lua
	static const auto RawUnlua =
#include "ScriptDependencyFile/UnLua.lua.inc"

	ScriptCode = FString(RawUnlua);

	if (!FPaths::FileExists(ScriptPath))
	{
		FFileHelper::SaveStringToFile(ScriptCode, *ScriptPath);
	}

	return false;
}

void FScriptHelperModule::Initialize(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow)
{

}

void FScriptHelperModule::AddGraphicsSwitcher(FToolBarBuilder& ToolBarBuilder)
{

}

void FScriptHelperModule::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE
