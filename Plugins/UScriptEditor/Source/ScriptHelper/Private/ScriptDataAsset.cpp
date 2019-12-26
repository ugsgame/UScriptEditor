#include "ScriptDataAsset.h"
#include "Misc/Paths.h"

UScriptDataAsset::UScriptDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	EFlag_IsValid = true;
#endif // WITH_EDITORONLY_DATA
}

FString UScriptDataAsset::GetDotPath() const
{
	FString DotPath = FPaths::GetPath(Path)/FPaths::GetBaseFilename(Path);
	DotPath = DotPath.Replace(TEXT("/"), TEXT("."));
	return DotPath;
}

FString UScriptDataAsset::GetPath() const
{
	return Path;
}

FString UScriptDataAsset::GetSourceCode() const
{
	return SourceCode;
}

TArray<uint8> UScriptDataAsset::GetByteCode() const
{
	return ByteCode;
}

