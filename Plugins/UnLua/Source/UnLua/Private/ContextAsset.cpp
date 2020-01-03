#include "ContextAsset.h"
#include "Misc/Paths.h"

UContextAsset::UContextAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	EFlag_IsValid = true;
#endif // WITH_EDITORONLY_DATA
}

FString UContextAsset::GetDotPath() const
{
	FString DotPath = FPaths::GetPath(Path)/FPaths::GetBaseFilename(Path);
	DotPath = DotPath.Replace(TEXT("/"), TEXT("."));
	return DotPath;
}

FString UContextAsset::GetPath() const
{
	return Path;
}

FString UContextAsset::GetSourceCode() const
{
	return SourceCode;
}

TArray<uint8> UContextAsset::GetByteCode() const
{
	return ByteCode;
}

