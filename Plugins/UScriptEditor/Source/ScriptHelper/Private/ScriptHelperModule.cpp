#include "ScriptHelperModule.h"
#include "ScriptHelperPrivatePCH.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "Runtime/SlateCore/Public/Rendering/SlateRenderer.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Modules/ModuleManager.h"
#include "ScriptHelperBPFunLib.h"
#include "Paths.h"
#include "FileHelper.h"

#include "UnLuaInterface.h"
#include "UnLuaDelegates.h"
#include "ScriptDataAsset.h"

#define LOCTEXT_NAMESPACE "ScriptHelper"

class FScriptHelperModule : public IScriptHelperModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	//bool OnLoadModuleContext(const FString& InModuleName,FModuleContext& InCodeContext);
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
	//SpawnSystemScriptFiles();
	//
	//FUnLuaDelegates::LoadModuleContext.BindRaw(this, &FScriptHelperModule::OnLoadModuleContext);
}

bool FScriptHelperModule::SpawnSystemScriptFiles()
{
	FString ScriptCode;
	FString ScriptPath = UScriptHelperBPFunLib::ScriptSourceDir() + "UnLua.lua";

	//UnLua.lua
	static const auto RawUnlua =
#include "ScriptDependencyFile/UnLua.lua.inc"

	ScriptCode = FString(RawUnlua);

	//if (!FPaths::FileExists(ScriptPath))
// 	{
// 		FFileHelper::SaveStringToFile(ScriptCode, *ScriptPath);
// 	}

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
/*
bool FScriptHelperModule::OnLoadModuleContext(const FString& InModuleName, FModuleContext& InCodeContext)
{
	FString ContextPath(InModuleName);
	FString ContextName = FPaths::GetCleanFilename(ContextPath);
	ContextPath = FString("/Game/") + ContextPath.Replace(TEXT("."), TEXT("/")) + "." + ContextName;
	UObject* TmpObject = LoadObject<UObject>(nullptr, *ContextPath);
	UScriptDataAsset* ContextAsset = Cast<UScriptDataAsset>(TmpObject);
	if (ContextAsset)
	{
		InCodeContext.Path = ContextAsset->GetPath();
		InCodeContext.SourceCode = ContextAsset->GetSourceCode();
		InCodeContext.ByteCode = ContextAsset->GetByteCode();

		return true;
	}
	return false;
}
*/
#undef LOCTEXT_NAMESPACE
