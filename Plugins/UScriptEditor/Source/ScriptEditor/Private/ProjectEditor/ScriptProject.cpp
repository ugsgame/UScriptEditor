
#include "ScriptProject.h"
#include "Misc/Paths.h"
#include "ScriptHelperBPFunLib.h"

UScriptProject::UScriptProject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Path = FPaths::ConvertRelativePathToFull(UScriptHelperBPFunLib::ScriptSourceDir());
}

