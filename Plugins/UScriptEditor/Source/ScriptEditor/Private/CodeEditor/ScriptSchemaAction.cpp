#include "ScriptSchemaAction.h"
#include "UScriptDebuggerSetting.h"
#include "UObjectIterator.h"
#include "MetaData.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "KismetEditorUtilities.h"
#include "ScriptParamCollection.h"

#define  LOCTEXT_NAMESPACE "ScriptActionCollecter"

USTRUCT()
struct FParameterData
{
	bool IsDefaultValve;
	FString ParamName;
	IParamValue* ParamValue;
	FString ValueString;

	FParameterData(bool IsDefaul, FString InParamName, IParamValue* InParamValue, FString InValueString)
		: IsDefaultValve(IsDefaul)
		, ParamName(InParamName)
		, ParamValue(InParamValue)
		, ValueString(InValueString)
	{}
};


static FString ConcatCategories(FString RootCategory, FString const& SubCategory)
{
	FString ConcatedCategory = MoveTemp(RootCategory);
	if (!SubCategory.IsEmpty() && !ConcatedCategory.IsEmpty())
	{
		ConcatedCategory += TEXT("|");
	}
	ConcatedCategory += SubCategory;

	return ConcatedCategory;
}

UScriptActionCollecter* UScriptActionCollecter::Get()
{
	static UScriptActionCollecter* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptActionCollecter>();
		Singleton->AddToRoot();
		Singleton->LuaActionList = new FGraphActionListBuilderBase();
		Singleton->ScriptActionList = new FGraphActionListBuilderBase();

		Singleton->Reflash();
	}
	return Singleton;
}

void UScriptActionCollecter::Reflash()
{
	CreateLuaActions();
	CreateScriptActions();
}

void UScriptActionCollecter::AddScriptAction(UClass* Class, bool InIsStatic, bool InIsFunction,FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString InCodeClip)
{
	TSharedPtr<FScriptSchemaAction> NewAction(new FScriptSchemaAction(FText::FromString(InNodeCategory), FText::FromString(InMenuDesc), FText::FromString(InToolTip), InCodeClip, InIsStatic, InIsFunction));
	NewAction->Class = Class;

	ScriptActions.Add(NewAction);
	ScriptActionList->AddAction(NewAction);
}

void UScriptActionCollecter::AddLuaAction(FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString InCodeClip)
{
	TSharedPtr<FScriptSchemaAction> NewAction(new FScriptSchemaAction(FText::FromString(InNodeCategory), FText::FromString(InMenuDesc), FText::FromString(InToolTip), InCodeClip,true,true));
	LuaActions.Add(NewAction);

	LuaActionList->AddAction(NewAction);
}

FString UScriptActionCollecter::GetAPICodeClip(UClass *Class, UFunction *Function,bool WithNote) const
{
	FString FullClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *Class->GetName());
	FString FunctionName = Function->GetName();

	TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Function);
	if (!MetaMap)
	{
		return "";
	}

	bool bIsStatic = Function->HasAllFunctionFlags(FUNC_Static);
	TArray<FParameterData> Parameters;

	for (TFieldIterator<UProperty> It(Function); It && (It->PropertyFlags & CPF_Parm); ++It)
	{
		UProperty *Property = *It;
		bool IsDefaultValve = true;

		// filter out properties without default value
		FName KeyName = FName(*FString::Printf(TEXT("CPP_Default_%s"), *Property->GetName()));
		FString *ValuePtr = MetaMap->Find(KeyName);
		if (!ValuePtr)
		{
			//continue;
			ValuePtr = new FString("");
			IsDefaultValve = false;
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
					Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FVectorParamValue(FVector(X, Y, Z), EScriptParamType::Vector), ValueStr));
				}
				else
				{
					Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FVectorParamValue(FVector(), EScriptParamType::Vector), ValueStr));
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
					Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FRotatorParamValue(FRotator(Pitch, Yaw, Roll), EScriptParamType::Rotator), ValueStr));
				}
				else
				{
					Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FRotatorParamValue(FRotator(0, 0, 0), EScriptParamType::Rotator), ValueStr));
				}
			}
			else if (StructProperty->Struct == Vector2DStruct)              // FVector2D
			{
				FVector2D Value;
				Value.InitFromString(ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FVector2DParamValue(Value, EScriptParamType::Vector2D), ValueStr));
			}
			else if (StructProperty->Struct == LinearColorStruct)           // FLinearColor
			{
				static FLinearColor ZeroLinearColor(ForceInit);
				FLinearColor Value;
				Value.InitFromString(ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FLinearColorParamValue(Value, EScriptParamType::LinearColor), ValueStr));
			}
			else if (StructProperty->Struct == ColorStruct)                 // FColor
			{
				static FColor ZeroColor(ForceInit);
				FColor Value;
				Value.InitFromString(ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FColorParamValue(Value, EScriptParamType::Color), ValueStr));
			}
		}
		else
		{
			if (Property->IsA(UIntProperty::StaticClass()))                 // int
			{
				int32 Value = TCString<TCHAR>::Atoi(*ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FIntParamValue(Value, EScriptParamType::Int), ValueStr));
			}
			else if (Property->IsA(UByteProperty::StaticClass()))           // byte
			{
				const UEnum *Enum = CastChecked<UByteProperty>(Property)->Enum;
				int32 Value = Enum ? (int32)Enum->GetValueByNameString(ValueStr) : TCString<TCHAR>::Atoi(*ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FByteParamValue(Value, EScriptParamType::Byte), ValueStr));
			}
			else if (Property->IsA(UEnumProperty::StaticClass()))           // enum
			{
				const UEnum *Enum = CastChecked<UEnumProperty>(Property)->GetEnum();
				int64 Value = Enum ? Enum->GetValueByNameString(ValueStr) : TCString<TCHAR>::Atoi64(*ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FEnumParamValue(Value, EScriptParamType::Enum), ValueStr));
			}
			else if (Property->IsA(UFloatProperty::StaticClass()))          // float
			{
				float Value = TCString<TCHAR>::Atof(*ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FFloatParamValue(Value, EScriptParamType::Float), ValueStr));
			}
			else if (Property->IsA(UDoubleProperty::StaticClass()))         // double
			{
				double Value = TCString<TCHAR>::Atod(*ValueStr);
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FDoubleParamValue(Value, EScriptParamType::Double), ValueStr));
			}
			else if (Property->IsA(UBoolProperty::StaticClass()))           // boolean
			{
				static FString FalseValue(TEXT("false"));
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FBoolParamValue(true, EScriptParamType::Bool), ValueStr));
			}
			else if (Property->IsA(UNameProperty::StaticClass()))           // FName
			{
				static FString NoneValue(TEXT("None"));
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FNameParamValue(*ValueStr, EScriptParamType::Name), ValueStr));
			}
			else if (Property->IsA(UTextProperty::StaticClass()))           // FText
			{
#if ENGINE_MINOR_VERSION > 20
				if (ValueStr.StartsWith(TEXT("INVTEXT(\"")))
				{
					Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FTextParamValue(FText::FromString(*ValueStr), EScriptParamType::Text), ValueStr));
				}
				else
#endif
				{
					Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FTextParamValue(FText::FromString(*ValueStr), EScriptParamType::Text), ValueStr));
				}
			}
			else if (Property->IsA(UStrProperty::StaticClass()))            // FString
			{
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FStringParamValue(*ValueStr, EScriptParamType::String), ValueStr));
			}
			else if (Property->IsA(UArrayProperty::StaticClass()))			// UArray
			{
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FTArrayParamValue(*ValueStr, EScriptParamType::Array), ValueStr));
			}
			else if (Property->IsA(UObjectProperty::StaticClass()))			// UObject
			{
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FUObjectParamValue(*ValueStr, EScriptParamType::Object), ValueStr));
			}
			else
			{
				//continue;
				Parameters.Add(FParameterData(IsDefaultValve, Property->GetName(), new FUnknowParamValue(*ValueStr, EScriptParamType::Unknow), ValueStr));
			}
		}
	}


	FString ParameterListString;
	FString ParameterNotesString;

	for (int32 i = 0; i < Parameters.Num(); i++)
	{
		FString StringType = "UnKnow";
		if (Parameters[i].ParamName != "ReturnValue")
		{		
			switch (Parameters[i].ParamValue->GetType())
			{
			case EScriptParamType::Bool:
				StringType = "Bool";
				ParameterListString += "true";
				break;
			case EScriptParamType::Byte:
				StringType = "Byte";
				ParameterListString += Parameters[i].ValueString;
				
				break;
			case EScriptParamType::Int:
				StringType = "Int";
				ParameterListString += Parameters[i].ValueString;
				
				break;
			case EScriptParamType::Enum:
				StringType = "Enum";
				ParameterListString += Parameters[i].ValueString;
				
				break;
			case EScriptParamType::Float:
				StringType = "Float";
				ParameterListString += Parameters[i].ValueString;
				
				break;
			case EScriptParamType::Double:
				StringType = "Double";
				ParameterListString += Parameters[i].ValueString;
				break;
			case EScriptParamType::Name:
				StringType = "Name";
				ParameterListString += Parameters[i].ValueString;			
				break;
			case EScriptParamType::Text:
				StringType = "Text";
				ParameterListString += Parameters[i].ValueString;			
				break;
			case EScriptParamType::String:
				StringType = "String";
				ParameterListString += FString::Printf(TEXT("\"%s\""), *Parameters[i].ValueString);
				break;
			case EScriptParamType::Vector:
				StringType = "Vector";
				ParameterListString += FString::Printf(TEXT("FVector(%s)"), *Parameters[i].ValueString);				
				break;
			case EScriptParamType::Vector2D:
				StringType = "FVector2D";
				ParameterListString += FString::Printf(TEXT("FVector2D(%s)"), *Parameters[i].ValueString);			
				break;
			case EScriptParamType::Rotator:
				StringType = "Rotator";
				ParameterListString += FString::Printf(TEXT("FRotator(%s)"), *Parameters[i].ValueString);
				break;
			case EScriptParamType::LinearColor:
				StringType = "LinearColor";
				ParameterListString += FString::Printf(TEXT("FLinearColor(%s)"), *Parameters[i].ValueString);				
				break;
			case EScriptParamType::Color:
				StringType = "Color";
				ParameterListString += FString::Printf(TEXT("FColor(%s)"), *Parameters[i].ValueString);			
				break;
			case EScriptParamType::Array:
				StringType = "Array";
				ParameterListString += FString::Printf(TEXT("[TArray]"));
				break;
			case EScriptParamType::Object:
				StringType = "Object";
				ParameterListString += FString::Printf(TEXT("[UObject]"));
				break;
			default:
				StringType = "Unknow";
				ParameterListString += FString::Printf(TEXT("[Unknow]"));
				break;
			}
			ParameterListString += ",";
		}

		ParameterNotesString += FString::Printf(TEXT("%s(%s)"), *Parameters[i].ParamName, *StringType);
		ParameterNotesString += ",";
		
	}
	ParameterListString.RemoveFromStart(",");
	ParameterNotesString.RemoveFromStart(",");
	ParameterListString.RemoveFromEnd(",");
	ParameterNotesString.RemoveFromEnd(",");

	ParameterNotesString = "-[[" + ParameterNotesString + "--]]";
	ParameterNotesString = WithNote ? ParameterNotesString : "";
	if (bIsStatic)
	{
		return FString::Printf(TEXT("UE4.%s.%s(%s)%s"),*FullClassName, *FunctionName, *ParameterListString, *ParameterNotesString);
	}
	else
	{
		return FString::Printf(TEXT("%s(%s)%s"), *FunctionName, *ParameterListString, *ParameterNotesString);
	}
}

FString UScriptActionCollecter::GetVarCodeClip(UClass *Class, UProperty *Property, bool WithNote /*= false*/) const
{
	FString CPPType = Property->GetCPPType();
	FString CPPName = Property->GetNameCPP();
	return CPPName;
}

TArray<TSharedPtr<FScriptSchemaAction>> UScriptActionCollecter::GetScriptActions()
{
	return ScriptActions;
}

TArray<TSharedPtr<FScriptSchemaAction>> UScriptActionCollecter::GetLuaActions()
{
	return LuaActions;
}

FGraphActionListBuilderBase* UScriptActionCollecter::GetScriptActionList()
{
	return ScriptActionList;
}

FGraphActionListBuilderBase* UScriptActionCollecter::GetLuaActionList()
{
	return LuaActionList;
}

void UScriptActionCollecter::CreateScriptActions()
{
	ScriptActions.Empty();
	ScriptActionList->Empty();

	//TODO:
	/*
	for (auto PromptNode : UScriptDebuggerSetting::Get()->ScriptPromptArray)
	{
		AddScriptAction(PromptNode.Category, PromptNode.MenuDesc, PromptNode.ToolTip, PromptNode.CodeClip);
	}
	*/

	//TODO:
	for (TObjectIterator<UClass> classIt; classIt; ++classIt)
	{
		AddActionByClass(*classIt);
	}

	/*
	FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
	FBlueprintActionDatabase::FActionRegistry const& ActionRegistry = ActionDatabase.GetAllActions();
	for (auto const& ActionEntry : ActionRegistry)
	{
		if (UObject *ActionObject = ActionEntry.Key.ResolveObjectPtr())
		{
			for (UBlueprintNodeSpawner const* NodeSpawner : ActionEntry.Value)
			{
				FBlueprintActionInfo BlueprintAction(ActionObject, NodeSpawner);
				const UObject* ActonOwner = BlueprintAction.GetActionOwner();
			}

		}

	}
	*/
}

void UScriptActionCollecter::CreateLuaActions()
{
	LuaActions.Empty();
	ScriptActionList->Empty();

	FString LuaCategory("LUA");

	AddLuaAction(LuaCategory, "assert", "", "assert(v[,message])");
	AddLuaAction(LuaCategory, "collectgarbage", "", "collectgarbage(opt[,arg])");
	AddLuaAction(LuaCategory, "dofile", "", "dofile(filename)");
	AddLuaAction(LuaCategory, "error", "", "error(message[,level])");
	AddLuaAction(LuaCategory, "_G", "", "_G");
	AddLuaAction(LuaCategory, "getfenv", "", "getfenv(f)");
	AddLuaAction(LuaCategory, "getmetatable", "", "getmetatable(object)");
	AddLuaAction(LuaCategory, "load", "", "load(func[,chunkname])");
	AddLuaAction(LuaCategory, "loadfile", "", "loadfile([filename])");
	AddLuaAction(LuaCategory, "loadstring", "", "loadstring(string[,chunkname])");
	AddLuaAction(LuaCategory, "next", "", "next(table[,index])");
	AddLuaAction(LuaCategory, "ipairs", "", "ipairs(t)");
	AddLuaAction(LuaCategory, "pcall", "", "pcall(f,arg1,…)");
	AddLuaAction(LuaCategory, "print", "", "print(…)");
	AddLuaAction(LuaCategory, "rawequal", "", "rawequal(v1,v2)");
	AddLuaAction(LuaCategory, "rawget", "", "rawget(table,index)");
	AddLuaAction(LuaCategory, "rawset", "", "rawset(table,index,value)");
	AddLuaAction(LuaCategory, "select", "", "select(index,…)");
	AddLuaAction(LuaCategory, "setfenv", "", "setfenv(f,table)");
	AddLuaAction(LuaCategory, "setmetatable", "", "setmetatable(table,metatable)");
	AddLuaAction(LuaCategory, "tonumber", "", "tonumber(e[,base])");
	AddLuaAction(LuaCategory, "tostring", "", "tostring(e)");
	AddLuaAction(LuaCategory, "type", "", "type(v)");
	AddLuaAction(LuaCategory, "unpack", "", "unpack(list[,i[,j]])");
	AddLuaAction(LuaCategory, "_VERSION", "", "_VERSION");
	AddLuaAction(LuaCategory, "xpcall", "", "xpcall(f,err)");
	//String 
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.byte", "", "string.byte(s[,i[,j]])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.char", "", "string.char(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.dump", "", "string.dump(function[,strip])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.find", "", "string.find(s,pattern[,init[,plain]])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.format", "", "string.format(formatstring[,value[,…]])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.math", "", "string.match(s,pattern[,init])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.gmatch", "", "string.gmatch(s,pattern)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.gsub", "", "string.gsub(s,pattern,repl[,n])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.len", "", "string.len(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.lower", "", "string.lower(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.upper", "", "string.upper(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.rep", "", "string.rep(s,n[,sep])");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.reverse", "", "string.reverse(s)");
	AddLuaAction(ConcatCategories(LuaCategory, "String"), "string.sub", "", "string.sub(s,i[,j])");

	//Math
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.abs", "", "math.abs(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.acos", "", "math.acos(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.asin", "", "math.asin(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.atan2", "", "math.atan2(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.atan", "", "math.atan(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.ceil", "", "math.ceil(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.cosh", "", "math.cosh(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.cos", "", "math.cos(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.deg", "", "math.deg(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.exp", "", "math.exp(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.floor", "", "math.floor(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.fmod", "", "math.fmod(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.frexp", "", "math.frexp(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.ldexp", "", "math.ldexp(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.log10", "", "math.log10(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.log", "", "math.log(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.max", "", "math.max(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.min", "", "math.min(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.modf", "", "math.modf(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.pow", "", "math.pow(x,y)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.random", "", "math.random(x[,y])");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.randomseed", "", "math.randomseed(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.sinh", "", "math.sinh(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.sin", "", "math.sin(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.sqrt", "", "math.sqrt(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.tanh", "", "math.tanh(x)");
	AddLuaAction(ConcatCategories(LuaCategory, "Math"), "math.tan", "", "math.tan(x)");

	//Table
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.concat", "", "table.concat(table[,sep[,i[,j]]])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.insert", "", "table.insert(table,[pos,]value)");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.move", "", "table.move(table1,f,e,t[,table2])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.pack", "", "table.pack(…)");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.remove", "", "table.remove(table[,pos])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.sort", "", "table.sort(table[,comp])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.unpack", "", "table.unpack(table[,i[,j]])");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.getn", "", "table.getn(table)");
	AddLuaAction(ConcatCategories(LuaCategory, "Table"), "table.maxn", "", "table.maxn(table)");
}

void UScriptActionCollecter::AddActionByClass(UClass* InClass, bool CategoryByClass)
{
	UClass* Class = InClass;
	FString ClassName = *Class->GetName();

	FString FullClassName = FString::Printf(TEXT("%s%s"), Class->GetPrefixCPP(), *ClassName);

	for (FString DefaultClass : GDefaultClassCollection)
	{
		if (DefaultClass == FullClassName)
		{
			//Add Functions
			for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); FuncIt/*&&(FuncIt->FunctionFlags & FUNC_BlueprintCallable&FUNC_BlueprintEvent&FUNC_BlueprintPure)*/; ++FuncIt)
			{

				UFunction *Function = *FuncIt;
				TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Function);
				if (!MetaMap)
				{
					continue;
				}

				//US_Log("Function:%s", *Function->GetName());
				FString FunctionName = *Function->GetName() + FString("()");

				FString *CategoryPtr = MetaMap->Find("Category");
				if (!CategoryPtr)
				{
					CategoryPtr = new FString("");
					//US_Log("Category:%s", *CategoryStr);
					CategoryPtr->Append(ClassName);
				}

				FString CategoryStr = FString(*CategoryPtr);

				FString *ToolTipPtr = MetaMap->Find("ToolTip");
				if (!ToolTipPtr)
				{
					ToolTipPtr = new FString(FunctionName);
				}
				FString ToolTipStr = FString(*ToolTipPtr);
				ToolTipStr += FString::Printf(TEXT("\n\nTarget is %s"), *FullClassName);

				FString CodeClip = GetAPICodeClip(Class, Function);
				CategoryStr = CategoryByClass ? ConcatCategories(ClassName, CategoryStr) : CategoryStr;

				//////////////////////////////////////////////////////////////////////////
				AddScriptAction(Class, Function->HasAllFunctionFlags(FUNC_Static),true,CategoryStr, FunctionName, ToolTipStr, CodeClip);
			
			}
			//@todo:Add Variables
			for (TFieldIterator<UProperty> ProIt(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated); ProIt/*&&(ProIt->PropertyFlags & CPF_BlueprintVisible&CPF_BlueprintReadOnly)*/; ++ProIt)
			{
				UProperty *Property = *ProIt;

				TMap<FName, FString> *MetaMap = UMetaData::GetMapForObject(Property);
				if (!MetaMap)
				{
					continue;
				}

				FString PropertyName = *Property->GetNameCPP();


				FString *CategoryPtr = MetaMap->Find("Category");
				if (!CategoryPtr)
				{
					CategoryPtr = new FString("");
					//US_Log("Category:%s", *CategoryStr);
					CategoryPtr->Append(ClassName);
				}

				FString CategoryStr = FString(*CategoryPtr);

				FString *ToolTipPtr = MetaMap->Find("ToolTip");
				if (!ToolTipPtr)
				{
					ToolTipPtr = new FString(PropertyName);
				}
				FString ToolTipStr = FString(*ToolTipPtr);
				ToolTipStr +=(FString::Printf(TEXT("\nType:%s"), *Property->GetCPPType()));
				ToolTipStr += FString::Printf(TEXT("\n\nTarget is %s"), *FullClassName);

				FString CodeClip = PropertyName;
				CategoryStr = CategoryByClass ? ConcatCategories(ClassName, CategoryStr) : CategoryStr;

				//////////////////////////////////////////////////////////////////////////
				AddScriptAction(Class, false, false, CategoryStr, PropertyName, ToolTipStr, CodeClip);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
