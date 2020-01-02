// Fill out your copyright notice in the Description page of Project Settings.

#include "ScriptHelperBPFunLib.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Algo/Count.h"
#include "LuaContext.h"
#include "UnLuaManager.h"
#include "LuaDynamicBinding.h"
#include "ScriptDataAsset.h"

#include "AssetRegistryModule.h"


bool UScriptHelperBPFunLib::GetFloatByName(UObject* Target, FName VarName, float &outFloat)
{
	if (Target) //make sure Target was set in blueprints. 
	{
		float FoundFloat;
		UFloatProperty* FloatProp = FindField<UFloatProperty>(Target->GetClass(), VarName);  // try to find float property in Target named VarName
		if (FloatProp) //if we found variable
		{
			FoundFloat = FloatProp->GetPropertyValue_InContainer(Target);  // get the value from FloatProp
			outFloat = FoundFloat;  // return float
			return true; // we can return
		}
	}
	return false; // we haven't found variable return false
}

bool UScriptHelperBPFunLib::GetScriptDataByName(UObject* Target, FName VarName, UScriptDataAsset* &outScriptData)
{
	if (Target) //make sure Target was set in blueprints. 
	{
		UScriptDataAsset* FoundScriptData = nullptr;
		UObjectProperty* ScriptProp = FindField<UObjectProperty>(Target->GetClass(), VarName);  // try to find float property in Target named VarName
		if (ScriptProp) //if we found variable
		{
			FoundScriptData = Cast<UScriptDataAsset>(ScriptProp->GetPropertyValue_InContainer(Target));  // get the value from FloatProp
			outScriptData = FoundScriptData;  // return float

			return FoundScriptData ? true:false;
		}
	}
	return false; // we haven't found variable return false
}

FString UScriptHelperBPFunLib::ScriptSourceRoot()
{
	return TEXT("Script/");
}

FString UScriptHelperBPFunLib::ScriptSourceDir()
{
	return FPaths::ProjectDir() + ScriptSourceRoot();
}

bool UScriptHelperBPFunLib::TryToRegisterScriptAsset(FString ModuleName)
{
	if (ModuleName.IsEmpty())return false;

	FString ScriptAssetPath = FString("/Game/") + ModuleName.Replace(TEXT("."), TEXT("/"));
	FString ScriptFileName = FPaths::GetBaseFilename(ScriptAssetPath);
	ScriptAssetPath += "."+ ScriptFileName;
	FString OutFailureReason;
	FString ObjectPath = ConvertAnyPathToObjectPath(ScriptAssetPath, OutFailureReason);
	UE_LOG(LogTemp, Log, TEXT("%s"), *OutFailureReason);

	if (ObjectPath.IsEmpty())
	{	
		return false;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*ObjectPath);
	if (!AssetData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("The AssetData '%s' could not be found in the Content Browser."), *ObjectPath);
		return false;
	}
	if (!AssetData.GetClass()->IsChildOf(UScriptDataAsset::StaticClass()))
	{
		//log(TEXT("Is not a ScriptData Asset"));
		UE_LOG(LogTemp, Error, TEXT("% is not a ScriptData Asset!"), *ScriptAssetPath);
		return false;
	}

	if (UScriptDataAsset* UScript = Cast<UScriptDataAsset>(AssetData.FastGetAsset(true)))
	{
		GCodeContext.Path = UScript->Path;
		GCodeContext.SourceCode = UScript->SourceCode;
		GCodeContext.ByteCode = UScript->ByteCode;

		UE_LOG(LogTemp, Log, TEXT("Registered (%s) Success"), *ScriptAssetPath);
		return true;
	}
	
	UE_LOG(LogTemp, Error, TEXT("Load %s fail!"), *ScriptAssetPath);
	return false;
}

bool UScriptHelperBPFunLib::TryToBindingScript(UObject* InObject, UScriptDataAsset *InScriptData)
{
	FLuaContext* Context = FLuaContext::Create();
	if (InObject &&  Context && Context->IsEnable() && Context->GetManager())
	{
		UClass *Class = InObject->GetClass();
		FString ModuleName;
		FCodeContext CodeContext;

		if (InScriptData)
		{
			ModuleName = InScriptData->GetDotPath();
			CodeContext.SourceCode = InScriptData->GetSourceCode();
			CodeContext.ByteCode = InScriptData->GetByteCode();
			CodeContext.Path = InScriptData->GetPath();
		}

		if (ModuleName.Len() < 1)
		{
			ModuleName = Class->GetName();
		}

		//Set Global context
		GCodeContext = CodeContext;
		return Context->GetManager()->Bind(InObject, Class, *ModuleName, GLuaDynamicBinding.InitializerTableRef);
	}
	return false;
}


FString UScriptHelperBPFunLib::ConvertAnyPathToObjectPath(const FString& AnyAssetPath, FString& OutFailureReason)
{
	if (AnyAssetPath.Len() < 2) // minimal length to have /G
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because the Root path need to be specified. ie /Game/"), *AnyAssetPath);
		return FString();
	}

	// Remove class name from Reference Path
	FString TextPath = FPackageName::ExportTextPathToObjectPath(AnyAssetPath);

	// Remove class name Fullname
	TextPath = RemoveFullName(TextPath, OutFailureReason);
	if (TextPath.IsEmpty())
	{
		return FString();
	}

	// Extract the subobject path if any
	FString SubObjectPath;
	int32 SubObjectDelimiterIdx;
	if (TextPath.FindChar(SUBOBJECT_DELIMITER_CHAR, SubObjectDelimiterIdx))
	{
		SubObjectPath = TextPath.Mid(SubObjectDelimiterIdx + 1);
		TextPath = TextPath.Left(SubObjectDelimiterIdx);
	}

	// Convert \ to /
	TextPath.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);
	FPaths::RemoveDuplicateSlashes(TextPath);

	// Get asset full name, i.e."PackageName.ObjectName:InnerAssetName.2ndInnerAssetName" from "/Game/Folder/PackageName.ObjectName:InnerAssetName.2ndInnerAssetName"
	FString AssetFullName;
	{
		// Get everything after the last slash
		int32 IndexOfLastSlash = INDEX_NONE;
		TextPath.FindLastChar('/', IndexOfLastSlash);

		FString Folders = TextPath.Left(IndexOfLastSlash);
		// Test for invalid characters
		if (!IsAValidPath(Folders, INVALID_LONGPACKAGE_CHARACTERS, OutFailureReason))
		{
			return FString();
		}

		AssetFullName = TextPath.Mid(IndexOfLastSlash + 1);
	}

	// Get the object name
	FString ObjectName = FPackageName::ObjectPathToObjectName(AssetFullName);
	if (ObjectName.IsEmpty())
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it doesn't contain an asset name."), *AnyAssetPath);
		return FString();
	}

	// Test for invalid characters
	if (!IsAValidPath(ObjectName, INVALID_OBJECTNAME_CHARACTERS, OutFailureReason))
	{
		return FString();
	}

	// Confirm that we have a valid Root Package and get the valid PackagePath /Game/MyFolder/MyAsset
	FString PackagePath;
	if (!FPackageName::TryConvertFilenameToLongPackageName(TextPath, PackagePath, &OutFailureReason))
	{
		return FString();
	}

	if (PackagePath.Len() == 0)
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert path '%s' because the PackagePath is empty."), *AnyAssetPath);
		return FString();
	}

	if (PackagePath[0] != TEXT('/'))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert path '%s' because the PackagePath '%s' doesn't start with a '/'."), *AnyAssetPath, *PackagePath);
		return FString();
	}

	FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *ObjectName);

	if (FPackageName::IsScriptPackage(ObjectPath))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it start with /Script/"), *AnyAssetPath);
		return FString();
	}
	if (FPackageName::IsMemoryPackage(ObjectPath))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it start with /Memory/"), *AnyAssetPath);
		return FString();
	}

	// Confirm that the PackagePath starts with a valid root
	if (!HasValidRoot(PackagePath))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it does not map to a root."), *AnyAssetPath);
		return FString();
	}

	return ObjectPath;
}

bool UScriptHelperBPFunLib::HasValidRoot(const FString& ObjectPath)
{
	FString Filename;
	bool bValidRoot = true;
	if (!ObjectPath.IsEmpty() && ObjectPath[ObjectPath.Len() - 1] == TEXT('/'))
	{
		bValidRoot = FPackageName::TryConvertLongPackageNameToFilename(ObjectPath, Filename);
	}
	else
	{
		FString ObjectPathWithSlash = ObjectPath;
		ObjectPathWithSlash.AppendChar(TEXT('/'));
		bValidRoot = FPackageName::TryConvertLongPackageNameToFilename(ObjectPathWithSlash, Filename);
	}

	return bValidRoot;
}

/** Remove Class from "Class /Game/MyFolder/MyAsset" */
FString UScriptHelperBPFunLib::RemoveFullName(const FString& AnyAssetPath, FString& OutFailureReason)
{
	FString Result = AnyAssetPath.TrimStartAndEnd();
	int32 NumberOfSpace = Algo::Count(AnyAssetPath, TEXT(' '));

	if (NumberOfSpace == 0)
	{
		return MoveTemp(Result);
	}
	else if (NumberOfSpace > 1)
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert path '%s' because there are too many spaces."), *AnyAssetPath);
		return FString();
	}
	else// if (NumberOfSpace == 1)
	{
		int32 FoundIndex = 0;
		AnyAssetPath.FindChar(TEXT(' '), FoundIndex);
		check(FoundIndex > INDEX_NONE && FoundIndex < AnyAssetPath.Len()); // because of TrimStartAndEnd

		// Confirm that it's a valid Class
		FString ClassName = AnyAssetPath.Left(FoundIndex);

		// Convert \ to /
		ClassName.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);

		// Test ClassName for invalid Char
		const int32 StrLen = FCString::Strlen(INVALID_OBJECTNAME_CHARACTERS);
		for (int32 Index = 0; Index < StrLen; ++Index)
		{
			int32 InvalidFoundIndex = 0;
			if (ClassName.FindChar(INVALID_OBJECTNAME_CHARACTERS[Index], InvalidFoundIndex))
			{
				OutFailureReason = FString::Printf(TEXT("Can't convert the path %s because it contains invalid characters (probably spaces)."), *AnyAssetPath);
				return FString();
			}
		}

		// Return the path without the Class name
		return AnyAssetPath.Mid(FoundIndex + 1);
	}
}

// Test for invalid characters
bool UScriptHelperBPFunLib::IsAValidPath(const FString& Path, const TCHAR* InvalidChar, FString& OutFailureReason)
{
	// Like !FName::IsValidGroupName(Path)), but with another list and no conversion to from FName
	// InvalidChar may be INVALID_OBJECTPATH_CHARACTERS or INVALID_LONGPACKAGE_CHARACTERS or ...
	const int32 StrLen = FCString::Strlen(InvalidChar);
	for (int32 Index = 0; Index < StrLen; ++Index)
	{
		int32 FoundIndex = 0;
		if (Path.FindChar(InvalidChar[Index], FoundIndex))
		{
			OutFailureReason = FString::Printf(TEXT("Can't convert the path %s because it contains invalid characters."), *Path);
			return false;
		}
	}

	if (Path.Len() > FPlatformMisc::GetMaxPathLength())
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path %s because it is too long; this may interfere with cooking for consoles. Unreal filenames should be no longer than %d characters."), *Path, FPlatformMisc::GetMaxPathLength());
		return false;
	}
	return true;
}
