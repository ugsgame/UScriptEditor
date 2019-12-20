// Tencent is pleased to support the open source community by making UnLua available.
// 
// Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the MIT License (the "License"); 
// you may not use this file except in compliance with the License. You may obtain a copy of the License at
//
// http://opensource.org/licenses/MIT
//
// Unless required by applicable law or agreed to in writing, 
// software distributed under the License is distributed on an "AS IS" BASIS, 
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
// See the License for the specific language governing permissions and limitations under the License.

#include "CoreUObject.h"
#include "Features/IModularFeatures.h"
#include "IScriptGeneratorPluginInterface.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "FScriptParamCollectorModule"

class FScriptParamCollectorModule : public IScriptGeneratorPluginInterface
{
public:
    virtual void StartupModule() override { IModularFeatures::Get().RegisterModularFeature(TEXT("ScriptGenerator"), this); }
    virtual void ShutdownModule() override { IModularFeatures::Get().UnregisterModularFeature(TEXT("ScriptGenerator"), this); }
    virtual FString GetGeneratedCodeModuleName() const override { return TEXT("ScriptEditor"); }
    virtual bool SupportsTarget(const FString& TargetName) const override { return true; }

    virtual bool ShouldExportClassesForModule(const FString& ModuleName, EBuildModuleType::Type ModuleType, const FString& ModuleGeneratedIncludeDirectory) const override
    {
        return ModuleType == EBuildModuleType::EngineRuntime /*|| ModuleType == EBuildModuleType::GameRuntime || ModuleType == EBuildModuleType::GameDeveloper*/;    // only 'EngineRuntime' and 'GameRuntime' are valid
    }
    
    virtual void Initialize(const FString& RootLocalPath, const FString& RootBuildPath, const FString& OutputDirectory, const FString& IncludeBase) override
    {
        GeneratedFileContent.Empty();
        GeneratedFileContent += FString::Printf(TEXT("FFunctionCollection *FC = nullptr;\r\n"));
        GeneratedFileContent += FString::Printf(TEXT("FParameterCollection *PC = nullptr;\r\n\r\n"));

        OutputDir = OutputDirectory;
    }

    virtual void ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged) override
    {
        // #lizard forgives

        // filter out interfaces
        if (Class->HasAnyClassFlags(CLASS_Interface))
        {
            return;
        }

        for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); FuncIt; ++FuncIt)
        {
            UFunction *Function = *FuncIt;
            CurrentFunctionName.Empty();

            // filter out functions without meta data
            TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Function);
            if (!MetaMap)
            {
                continue;
            }

			PreAddProperty(Class, Function);
            // parameters
            for (TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags & CPF_Parm ); ++It)
            {
                UProperty *Property = *It;

                // filter out properties without default value
                FName KeyName = FName(*FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName()));
                FString *ValuePtr = MetaMap->Find(KeyName);
                if (!ValuePtr)
                {
                    //continue;
					ValuePtr = new FString("");
                }

                const FString &ValueStr = *ValuePtr;
                if (Property->IsA(UStructProperty::StaticClass()))
                {
                    // get all possible script structs
                    UPackage* CoreUObjectPackage = UObject::StaticClass()->GetOutermost();
                    static const UScriptStruct* VectorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector"));
                    static const UScriptStruct* Vector2DStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Vector2D"));
                    static const UScriptStruct* RotatorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Rotator"));
                    static const UScriptStruct* LinearColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("LinearColor"));
                    static const UScriptStruct* ColorStruct = FindObjectChecked<UScriptStruct>(CoreUObjectPackage, TEXT("Color"));

                    const UStructProperty *StructProperty = CastChecked<UStructProperty>(Property);
                    if (StructProperty->Struct == VectorStruct)                     // FVector
                    {
                        TArray<FString> Values;
                        ValueStr.ParseIntoArray(Values, TEXT(","));
                        if (Values.Num() == 3)
                        {
                            float X = TCString<TCHAR>::Atof(*Values[0]);
                            float Y = TCString<TCHAR>::Atof(*Values[1]);
                            float Z = TCString<TCHAR>::Atof(*Values[2]);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVectorParamValue(FVector(%ff,%ff,%ff)));\r\n"), *Property->GetName(), X, Y, Z);
                        }
						else
						{
							GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVectorParamValue(FVector(%ff,%ff,%ff)));\r\n"), *Property->GetName(), 0.f, 0.f, 0.f);
						}
                    }
                    else if (StructProperty->Struct == RotatorStruct)               // FRotator
                    {
                        TArray<FString> Values;
                        ValueStr.ParseIntoArray(Values, TEXT(","));
                        if (Values.Num() == 3)
                        {
                            float Pitch = TCString<TCHAR>::Atof(*Values[0]);
                            float Yaw = TCString<TCHAR>::Atof(*Values[1]);
                            float Roll = TCString<TCHAR>::Atof(*Values[2]);
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FRotatorParamValue(FRotator(%ff,%ff,%ff)));\r\n"), *Property->GetName(), Pitch, Yaw, Roll);
                        }
						else
						{
							GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FRotatorParamValue(FRotator(%ff,%ff,%ff)));\r\n"), *Property->GetName(), 0.f, 0.f, 0.f);
						}
                    }
                    else if (StructProperty->Struct == Vector2DStruct)              // FVector2D
                    {
                        FVector2D Value;
                        Value.InitFromString(ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FVector2DParamValue(FVector2D(%ff,%ff)));\r\n"), *Property->GetName(), Value.X, Value.Y);
                    }
                    else if (StructProperty->Struct == LinearColorStruct)           // FLinearColor
                    {
                        static FLinearColor ZeroLinearColor(ForceInit);
                        FLinearColor Value;
                        Value.InitFromString(ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FLinearColorParamValue(FLinearColor(%ff,%ff,%ff,%ff)));\r\n"), *Property->GetName(), Value.R, Value.G, Value.B, Value.A);
                    }
                    else if (StructProperty->Struct == ColorStruct)                 // FColor
                    {
                        static FColor ZeroColor(ForceInit);
                        FColor Value;
                        Value.InitFromString(ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FColorParamValue(FColor(%d,%d,%d,%d)));\r\n"), *Property->GetName(), Value.R, Value.G, Value.B, Value.A);
                    }
                }
                else
                {
                    if (Property->IsA(UIntProperty::StaticClass()))                 // int
                    {
                        int32 Value = TCString<TCHAR>::Atoi(*ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FIntParamValue(%d));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(UByteProperty::StaticClass()))           // byte
                    {
                        const UEnum *Enum = CastChecked<UByteProperty>(Property)->Enum;
                        int32 Value = Enum ? (int32)Enum->GetValueByNameString(ValueStr) : TCString<TCHAR>::Atoi(*ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FByteParamValue(%d));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(UEnumProperty::StaticClass()))           // enum
                    {
                        const UEnum *Enum = CastChecked<UEnumProperty>(Property)->GetEnum();
                        int64 Value = Enum ? Enum->GetValueByNameString(ValueStr) : TCString<TCHAR>::Atoi64(*ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FEnumParamValue(%ld));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(UFloatProperty::StaticClass()))          // float
                    {
                        float Value = TCString<TCHAR>::Atof(*ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FFloatParamValue(%ff));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(UDoubleProperty::StaticClass()))         // double
                    {
                        double Value = TCString<TCHAR>::Atod(*ValueStr);
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FDoubleParamValue(%lf));\r\n"), *Property->GetName(), Value);
                    }
                    else if (Property->IsA(UBoolProperty::StaticClass()))           // boolean
                    {
                        static FString FalseValue(TEXT("false"));
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FBoolParamValue(true));\r\n"), *Property->GetName());
                    }
                    else if (Property->IsA(UNameProperty::StaticClass()))           // FName
                    {
                        static FString NoneValue(TEXT("None"));
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FNameParamValue(FName(\"%s\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
                    else if (Property->IsA(UTextProperty::StaticClass()))           // FText
                    {
#if ENGINE_MINOR_VERSION > 20
                        if (ValueStr.StartsWith(TEXT("INVTEXT(\"")))
                        {
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTextParamValue(%s));\r\n"), *Property->GetName(), *ValueStr);
                        }
                        else
#endif
                        {
                            GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTextParamValue(FText::FromString(TEXT(\"%s\"))));\r\n"), *Property->GetName(), *ValueStr);
                        }
                    }
                    else if (Property->IsA(UStrProperty::StaticClass()))            // FString
                    {
                        GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FStringParamValue(TEXT(\"%s\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
					else if (Property->IsA(UArrayProperty::StaticClass()))
					{
						GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FTArrayParamValue(TEXT(\"%s(TArrayType)\")));\r\n"), *Property->GetName(), *ValueStr);
					}
					else if (Property->IsA(UObjectProperty::StaticClass()))
					{
						GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FUObjectParamValue(TEXT(\"%s(UObjectType)\")));\r\n"), *Property->GetName(), *ValueStr);
					}
                    else
                    {
                        //continue;
						GeneratedFileContent += FString::Printf(TEXT("PC->Parameters.Add(TEXT(\"%s\"), new FUnknowParamValue(TEXT(\"%s(UnknowType)\")));\r\n"), *Property->GetName(), *ValueStr);
                    }
                }
            }
        }

        if (CurrentClassName.Len() > 0)
        {
            CurrentClassName.Empty();
            CurrentFunctionName.Empty();

            GeneratedFileContent += TEXT("\r\n");
        }
    }

    virtual void FinishExport() override
    {
        const FString FilePath = FString::Printf(TEXT("%s%s"), *OutputDir, TEXT("UScriptParamCollection.inl"));
        FString FileContent;
        FFileHelper::LoadFileToString(FileContent, *FilePath);
        if (FileContent != GeneratedFileContent)
        {
            bool bResult = FFileHelper::SaveStringToFile(GeneratedFileContent, *FilePath);
            check(bResult);
        }
    }

    virtual FString GetGeneratorName() const override
    {
        return TEXT("UScript Parameters Collector");
    }

private:
    void PreAddProperty(UClass *Class, UFunction *Function)
    {
        if (CurrentClassName.Len() < 1)
        {
            CurrentClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
            GeneratedFileContent += FString::Printf(TEXT("FC = &GDefaultParamCollection.Add(TEXT(\"%s\"));\r\n"), *CurrentClassName);
        }
        if (CurrentFunctionName.Len() < 1)
        {
            CurrentFunctionName = Function->GetName();
            GeneratedFileContent += FString::Printf(TEXT("PC = &FC->Functions.Add(TEXT(\"%s\"));\r\n"), *CurrentFunctionName);
        }

		FString ClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
		// filter out functions without meta data
		TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Function);
		if (MetaMap)
		{
			//////////////////////////////////////////////////////////////////////////
			FString *CategoryPtr = MetaMap->Find("Category");
			if (!CategoryPtr)
			{
				CategoryPtr = new FString("");
				CategoryPtr->Append(ClassName);
			}

			FString CategoryStr = FString(*CategoryPtr);
			GeneratedFileContent += FString::Printf(TEXT("PC->Category = TEXT(\"%s\");\r\n"), *CategoryStr);
			//////////////////////////////////////////////////////////////////////////

			//////////////////////////////////////////////////////////////////////////		                                                          
			FString *ToolTipPtr = MetaMap->Find("ToolTip");
			if (!ToolTipPtr)
			{
				ToolTipPtr = new FString("");
			}
			FString ToolTipStr = FString(*ToolTipPtr);
			ToolTipStr = ToolTipStr.Replace(TEXT("\\"), TEXT("\\\\"));
			ToolTipStr = ToolTipStr.Replace(TEXT("\n"), TEXT("\\n \\\n\t"));
			ToolTipStr = ToolTipStr.Replace(TEXT("\""), TEXT("\\\""));

			//ToolTipStr = ToolTipStr.Replace(TEXT("\c"), TEXT("c"));

			GeneratedFileContent += FString::Printf(TEXT("PC->ToolTip = TEXT(\"%s\");\r\n"), *ToolTipStr);
			//////////////////////////////////////////////////////////////////////////

		}
	}

    FString OutputDir;
    FString CurrentClassName;
    FString CurrentFunctionName;
    FString GeneratedFileContent;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FScriptParamCollectorModule, ScriptParamCollector)
