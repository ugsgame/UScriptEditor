#include "ScriptHook.h"
#include "ScriptHookClient.h"


IMPLEMENT_MODULE(FScriptHookModule, ScriptHook);

void FScriptHookModule::StartupModule()
{
#if(!WITH_EDITOR)
	//UScriptHookClient::Get();
#endif
}

void FScriptHookModule::ShutdownModule()
{
	//UScriptHookClient::Get()->UnBindDebugState();
}