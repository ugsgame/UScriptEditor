
#include "ScriptEditorSetting.h"

UScriptEdtiorSetting* UScriptEdtiorSetting::Get()
{
	static UScriptEdtiorSetting* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptEdtiorSetting>();
		Singleton->AddToRoot();
	}
	return Singleton;
}

