#include "ScriptHook.h"
#include "ScriptHookClient.h"


IMPLEMENT_MODULE(FScriptHookModule, ScriptHook);

void FScriptHookModule::StartupModule()
{
//TODO:开启远程调试很慢，后放到插件配置里开启
#if(!WITH_EDITOR)				
	//UScriptHookClient::Get();
#endif
}

void FScriptHookModule::ShutdownModule()
{
	//UScriptHookClient::Get()->UnBindDebugState();
}