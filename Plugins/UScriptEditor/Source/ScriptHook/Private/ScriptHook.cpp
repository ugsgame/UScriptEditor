#include "ScriptHook.h"
#include "ScriptHookClient.h"


IMPLEMENT_MODULE(FScriptHookModule, ScriptHook);

void FScriptHookModule::StartupModule()
{
	UScriptHookClient::Get();
}

void FScriptHookModule::ShutdownModule()
{
	//UScriptHookClient::Get()->UnBindDebugState();
}