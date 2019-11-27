
#include "ScriptProject.h"
#include "Misc/Paths.h"

UScriptProject::UScriptProject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Path = FPaths::ProjectContentDir();
}

