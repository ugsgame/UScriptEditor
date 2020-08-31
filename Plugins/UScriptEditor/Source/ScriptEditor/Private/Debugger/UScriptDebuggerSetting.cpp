// Fill out your copyright notice in the Description page of Project Settings.

#include "UScriptDebuggerSetting.h"
#include "UnLuaDelegates.h"
#include "UnLua.h"
#include "SScriptDebugger.h"
#include "UnLua/Private/ReflectionUtils/ClassDesc.h"
#include "UnLua/Private/ReflectionUtils/FunctionDesc.h"
#include "UnLua/Private/ReflectionUtils/PropertyDesc.h"
#include "UnLua/Private/LuaContext.h"
#include "GameFramework/Actor.h"
#include "ScriptEditor.h"


UScriptDebuggerSetting* UScriptDebuggerSetting::Get()
{
	static UScriptDebuggerSetting* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptDebuggerSetting>();
		Singleton->AddToRoot();

		FUnLuaDelegates::OnObjectBinded.AddUObject(Singleton, &UScriptDebuggerSetting::OnObjectBinded);
	}
	return Singleton;
}

void UScriptDebuggerSetting::SetTabIsOpen(bool IsOpen)
{
	bTabIsOpen = IsOpen;

	if (bTabIsOpen)
	{
		BindDebugState();
	}
	else
	{
		UnBindDebugState();
	}
}


/********Hook Debug Statement********/

//#define UNLUA_DEBUG
#ifdef UNLUA_DEBUG
static unlua_Debug ur;
#else
static unlua_State ur;
#endif // UNLUA_DEBUG

//#define UNFOLD_FUNCTION

const FString UScriptDebuggerSetting::TempVarName(TEXT("(*temporary)"));

const FString UScriptDebuggerSetting::SelfName(TEXT("self"));

const FString UScriptDebuggerSetting::OverriddenName(TEXT("Overridden"));

const FText UScriptDebuggerSetting::UFunctionText = FText::FromString(TEXT("UFunction"));

const FText UScriptDebuggerSetting::ClassDescText = FText::FromString(TEXT("ClassDesc"));

const FString UScriptDebuggerSetting::SelfLocationName(TEXT("SelfLocation"));

const FString UScriptDebuggerSetting::SelfRotatorName(TEXT("SelfRotator"));

const FString UScriptDebuggerSetting::SelfScalerName(TEXT("SelfScaler"));

const FString UScriptDebuggerSetting::FVectorName(TEXT("FVector"));

const FString UScriptDebuggerSetting::FRotatorName(TEXT("FRotator"));

const FString UScriptDebuggerSetting::ReturnValueName(TEXT("ReturnValue"));

const FText UScriptDebuggerSetting::SelfLocationText = FText::FromString(SelfLocationName);

const FText UScriptDebuggerSetting::SelfRotatorText = FText::FromString(SelfRotatorName);

const FText UScriptDebuggerSetting::SelfScalerText = FText::FromString(SelfScalerName);

const FText UScriptDebuggerSetting::FVectorText = FText::FromString("FVector");

const FText UScriptDebuggerSetting::FRotatorText = FText::FromString("FRotator");

static void debugger_hook_c(lua_State *L, lua_Debug *ar);

/********Hook Debug Statement********/

void UScriptDebuggerSetting::RegisterLuaState(lua_State* State)
{
	L = State;

#if 0

	UE_LOG(LogTemp, Log, TEXT("unlua_Debug RegisterLuaState"));

	lua_sethook(L, debugger_hook_c, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);

#else
	if (FScriptEditor::Get()->EnableBreakPoint.Num() > 0)
	{
		//Bind Lua Hook
		lua_sethook(L, debugger_hook_c, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);

#ifdef UNLUA_DEBUG
		for (TMap<FString, TSet<int32>>::TIterator It(FScriptEditor::Get()->EnableBreakPoint); It; ++It)
		{
			for (auto Line : It->Value)
			{
				UE_LOG(LogTemp, Log, TEXT("unlua_Debug BreakPoints file[%s] line[%i]"), *It->Key, Line);
			}
		}
#endif
	}
#endif

	hook_mode = EHookMode::H_Continue;

	//empty ScriptPromptGroup
	ScriptPromptGroup.Empty();

#if 0

	FString ThirdPersonGameModeName(TEXT("ThirdPersonGameMode"));
	FString ThirdPersonCharacterName(TEXT("ThirdPersonCharacter"));

	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Class = *ClassIt;

		UE_LOG(LogTemp, Log, TEXT("Iter Type[%d] Class[%s]"), Class->ClassFlags, *Class->GetName());

		if (Class->GetName().Contains(ThirdPersonGameModeName) || Class->GetName().Contains(ThirdPersonCharacterName))
		{

			//get function
			for (TFieldIterator<UFunction> FunIt(Class); FunIt; ++FunIt)
			{
				UFunction* Function = *FunIt;

				UE_LOG(LogTemp, Log, TEXT("Iter Class[%s] Func[%s]"), *Class->GetName(), *Function->GetName());

			}
		}
	}

#endif

}

void UScriptDebuggerSetting::UnRegisterLuaState(bool bFullCleanup)
{
#ifdef UNLUA_DEBUG
	if (bFullCleanup)
	{
		UE_LOG(LogTemp, Log, TEXT("unlua_Debug UnRegisterLuaState FullCleanup"));
	}
	else
		UE_LOG(LogTemp, Log, TEXT("unlua_Debug UnRegisterLuaState Not FullCleanup"));
#endif

	if (!bFullCleanup)
		return;

	if (L == NULL)
		return;

	lua_sethook(L, NULL, 0, 0);
	// Clear Debug Info
	SScriptDebugger::Get()->CleanDebugInfo();

	if (bFullCleanup)
	{
		hook_mode = EHookMode::H_Continue;
		L = NULL;
	}


	ScriptPromptArray.Empty();

	for (TMap<FString, TMap<FString, FScriptPromptNode>>::TIterator It(ScriptPromptGroup); It; ++It)
	{
		/*for (TMap<FString, FScriptPromptNode>::TIterator Ih(It->Value); Ih; ++Ih)
		{
			UE_LOG(LogTemp, Log, TEXT("Prompt key[%s] func[%s] value[%s]"), *It->Key, *Ih->Key , *Ih->Value.ToString());
		}*/

		TArray<FScriptPromptNode> TempArray;

		It->Value.GenerateValueArray(TempArray);

		ScriptPromptArray.Append(TempArray);
	}

#if 0
	//for(auto Item : ScriptPromptArray)
	//	UE_LOG(LogTemp, Log, TEXT("Prompt [%s]"), *Item.ToString());

	FLuaContext* LuaContext = FLuaContext::Create();

	TMap<FName, UnLua::IExportedClass*> ExportedReflectedClasses = LuaContext->GetExportedReflectedClasses();
	TMap<FName, UnLua::IExportedClass*> ExportedNonReflectedClasses = LuaContext->GetExportedNonReflectedClasses();
	TArray<UnLua::IExportedEnum*> ExportedEnums = LuaContext->GetExportedEnums();
	TArray<UnLua::IExportedFunction*> ExportedFunctions = LuaContext->GetExportedFunctions();

	for (TMap<FName, UnLua::IExportedClass*>::TIterator It(ExportedReflectedClasses); It; ++It)
	{
		UE_LOG(LogTemp, Log, TEXT("Prompt ReClass [%s] [%s]"), *It->Key.ToString(), *It->Value->GetName().ToString());
	}

	for (TMap<FName, UnLua::IExportedClass*>::TIterator It(ExportedNonReflectedClasses); It; ++It)
	{
		UE_LOG(LogTemp, Log, TEXT("Prompt NoReClass [%s] [%s]"), *It->Key.ToString(), *It->Value->GetName().ToString());
	}

	for (auto Item : ExportedEnums)
	{
		UE_LOG(LogTemp, Log, TEXT("Prompt Enum [%s]"), *Item->GetName());
	}

	for (auto Item : ExportedFunctions)
	{
		UE_LOG(LogTemp, Log, TEXT("Prompt Func [%s]"), *Item->GetName());
	}
#endif

}

bool UScriptDebuggerSetting::NameTranslate(int32 KindType, FString& VarName, int32 StackIndex)
{
	switch (KindType)
	{
	case LUA_TNUMBER:
		VarName = FString::Printf(TEXT("[%lf]"), lua_tonumber(L, StackIndex));
		return true;
	case LUA_TSTRING:
		VarName = UTF8_TO_TCHAR(lua_tostring(L, StackIndex));
		return true;
	}
	return false;
}

void UScriptDebuggerSetting::ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex)
{
	VarValue.Empty();
	switch (KindType)
	{
	case LUA_TNONE:
		//VarValue = TEXT("LUA_TNONE");
		VarType = TEXT("LUA_TNONE");
		break;
	case LUA_TNIL:
		//VarValue = TEXT("LUA_TNIL");
		VarType = TEXT("LUA_TNIL");
		break;
	case LUA_TBOOLEAN:
		VarValue = lua_toboolean(L, StackIndex) == 0 ? TEXT("false") : TEXT("true");
		VarType = TEXT("LUA_TBOOLEAN");
		break;
	case LUA_TLIGHTUSERDATA:
		VarValue = FString::Printf(TEXT("%p"), lua_touserdata(L, StackIndex));
		VarType = TEXT("LUA_TLIGHTUSERDATA");
		break;
	case LUA_TNUMBER:
		VarValue = FString::Printf(TEXT("%lf"), lua_tonumber(L, StackIndex));
		VarType = TEXT("LUA_TNUMBER");
		break;
	case LUA_TSTRING:
		VarValue = UTF8_TO_TCHAR(lua_tostring(L, StackIndex));
		VarType = TEXT("LUA_TSTRING");
		break;
	case LUA_TTABLE:
		//VarValue = TEXT("LUA_TTABLE");
		VarType = TEXT("LUA_TTABLE");
		break;
	case LUA_TFUNCTION:
		VarValue = FString::Printf(TEXT("%p"), lua_tocfunction(L, StackIndex));
		VarType = TEXT("LUA_TFUNCTION");
		break;
	case LUA_TUSERDATA:
		VarValue = FString::Printf(TEXT("%p"), lua_touserdata(L, StackIndex));
		VarType = TEXT("LUA_TUSERDATA");
		break;
	case LUA_TTHREAD:
		//VarValue = TEXT("LUA_TTHREAD");
		VarType = TEXT("LUA_TTHREAD");
		break;
	case LUA_NUMTAGS:
		//VarValue = TEXT("LUA_NUMTAGS");
		VarType = TEXT("LUA_NUMTAGS");
		break;
	}
}

void UScriptDebuggerSetting::IteraionTable(FScriptDebuggerVarNode& InNode, int32 NameIndex)
{
	FString VarName;
	FString VarValue;
	FString VarType;
	lua_pushnil(L);

	//check is last index
	if (InNode.NameList.Num() == NameIndex)
	{
		//Generate New Node
		while (lua_next(L, -2))
		{
			int32 NameType = lua_type(L, -2);

			if (NameTranslate(NameType, VarName, -2))
			{
				if (!TempVarName.Equals(VarName))
				{
					int ValueType = lua_type(L, -1);
					ValueTranslate(ValueType, VarValue, VarType, -1);

					if (!InNode.NodeChildren.Contains(VarName))
					{

						FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
						NewNode->NodeType = InNode.NodeType;
						NewNode->StackLevel = InNode.StackLevel;
						NewNode->KindType = ValueType;
						NewNode->VarName = FText::FromString(VarName);
						NewNode->VarValue = FText::FromString(VarValue);
						NewNode->VarType = FText::FromString(VarType);
						NewNode->NameList.Append(InNode.NameList);
						NewNode->NameList.Add(VarName);

						InNode.NodeChildren.Add(VarName, NewNode);
					}

				}
			}

			lua_pop(L, 1);
		}


	}
	else
	{
		// Continue IteraionTable
		while (lua_next(L, -2) != 0)
		{
			int32 NameType = lua_type(L, -2);

			if (NameTranslate(NameType, VarName, -2))
			{
				int ValueType = lua_type(L, -1);
				if (VarName.Equals(InNode.NameList[NameIndex]) && ValueType == LUA_TTABLE)
				{
					IteraionTable(InNode, NameIndex + 1);

					lua_pop(L, 1);

					break;
				}
			}

			lua_pop(L, 1);
		}
	}

	lua_pop(L, 1);
}

void UScriptDebuggerSetting::LocalListen(FScriptDebuggerVarNode& InNode)
{
	//Get Local Vars
	lua_Debug ar;

	if (lua_getstack(L, InNode.StackLevel, &ar) != 0)
	{
		int i = 1;
		const char* VarName;
		FString VarValue;
		FString VarType;
		while ((VarName = lua_getlocal(L, &ar, i)) != NULL)
		{
			if (TempVarName.Equals(UTF8_TO_TCHAR(VarName)))
			{
				lua_pop(L, 1);
				i++;
				continue;
			}


			int32 KindType = lua_type(L, -1);
			ValueTranslate(KindType, VarValue, VarType, -1);

			if (InNode.NameList.Num() == 0)
			{
				if (!InNode.NodeChildren.Contains(VarName))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = EScriptVarNodeType::V_Local;
					NewNode->StackLevel = InNode.StackLevel;
					NewNode->KindType = KindType;
					NewNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarName));
					NewNode->VarValue = FText::FromString(VarValue);
					NewNode->VarType = FText::FromString(VarType);
					NewNode->NameList.Add(UTF8_TO_TCHAR(VarName));
					InNode.NodeChildren.Add(VarName, NewNode);
				}
			}
			else
			{
				if (InNode.NameList[0].Equals(UTF8_TO_TCHAR(VarName)))
				{
					if (KindType == LUA_TTABLE)
					{
						IteraionTable(InNode, 1);
						lua_pop(L, 1);
						break;
					}
				}
			}

			lua_pop(L, 1);

			i++;
		}
	}
}

void UScriptDebuggerSetting::UpvalueListen(FScriptDebuggerVarNode& InNode)
{
	int i = 1;
	const char* VarName;
	FString VarValue;
	FString VarType;
	while ((VarName = lua_getupvalue(L, -1, i)) != NULL)
	{

		if (TempVarName.Equals(UTF8_TO_TCHAR(VarName)))
		{
			lua_pop(L, 1);
			i++;
			continue;
		}

		int KindType = lua_type(L, -1);

		ValueTranslate(KindType, VarValue, VarType, -1);

		if (InNode.NameList.Num() == 0)
		{
			if (!InNode.NodeChildren.Contains(VarName))
			{
				FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
				NewNode->NodeType = EScriptVarNodeType::V_UpValue;
				NewNode->StackLevel = InNode.StackLevel;
				NewNode->KindType = KindType;
				NewNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarName));
				NewNode->VarValue = FText::FromString(VarValue);
				NewNode->VarType = FText::FromString(VarType);
				NewNode->NameList.Add(UTF8_TO_TCHAR(VarName));
				InNode.NodeChildren.Add(VarName, NewNode);
			}
		}
		else
		{
			if (InNode.NameList[0].Equals(UTF8_TO_TCHAR(VarName)))
			{
				if (KindType == LUA_TTABLE)
				{
					IteraionTable(InNode, 1);
					lua_pop(L, 1);
					break;
				}
			}
		}

		lua_pop(L, 1);

		i++;
	}
}

void UScriptDebuggerSetting::GlobalListen(FScriptDebuggerVarNode& InNode)
{
	lua_pushglobaltable(L);
	lua_pushnil(L);
	const char* VarName;
	FString VarValue;
	FString VarType;
	int i = 0;
	while (lua_next(L, -2) != 0)
	{
		VarName = lua_tostring(L, -2);

		if (TempVarName.Equals(UTF8_TO_TCHAR(VarName)))
		{
			lua_pop(L, 1);
			i++;
			continue;
		}

		int KindType = lua_type(L, -1);

		ValueTranslate(KindType, VarValue, VarType, -1);

		if (InNode.NameList.Num() == 0)
		{
			if (!InNode.NodeChildren.Contains(VarName))
			{
				FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
				NewNode->NodeType = EScriptVarNodeType::V_Global;
				NewNode->StackLevel = InNode.StackLevel;
				NewNode->KindType = KindType;
				NewNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarName));
				NewNode->VarValue = FText::FromString(VarValue);
				NewNode->VarType = FText::FromString(VarType);
				NewNode->NameList.Add(UTF8_TO_TCHAR(VarName));
				InNode.NodeChildren.Add(VarName, NewNode);
			}
		}
		else
		{
			if (InNode.NameList[0].Equals(UTF8_TO_TCHAR(VarName)))
			{
				if (KindType == LUA_TTABLE)
				{
					IteraionTable(InNode, 1);
					lua_pop(L, 1);
					break;
				}
			}
		}

		lua_pop(L, 1);
		i++;
	}
	lua_pop(L, 1);
}

bool UScriptDebuggerSetting::PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, FProperty* Property, UObject* Object)
{
	//reset data
	VarValue.Empty();
	KindType = (int32)EScriptUEKindType::T_Single;
	VarPtr = NULL;

	FString CPPType = Property->GetCPPType();
	void* ValuePtr;

	if (CastField<FNumericProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FNumericProperty -- %s"), *CPPType);

		FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);
		ValuePtr = NumericProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = NumericProperty->GetNumericPropertyValueToString(ValuePtr);
	}
	else if (CastField<FEnumProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FEnumProperty -- %s"), *CPPType);

		FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property);
		ValuePtr = EnumProperty->ContainerPtrToValuePtr<uint8>(Object);
		EnumProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
	}
	else if (CastField<FBoolProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FBoolProperty -- %s"), *CPPType);

		FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property);
		ValuePtr = BoolProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("True") : TEXT("False");
	}
	else if (CastField<FObjectPropertyBase>(Property))
	{
		VarType = FString::Printf(TEXT("FObjectPropertyBase -- %s"), *CPPType);

		FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(Property);
		ValuePtr = ObjectPropertyBase->ContainerPtrToValuePtr<uint8>(Object);
		UObject* ObjectValue = ObjectPropertyBase->GetObjectPropertyValue(ValuePtr);
		if (ObjectValue)
		{
			if (Cast<AActor>(ObjectValue))
				KindType = (int32)EScriptUEKindType::T_AActor;
			else
				KindType = (int32)EScriptUEKindType::T_UObject;
			VarPtr = (void*)ObjectValue;
		}
	}
	else if (CastField<FSoftObjectProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FSoftObjectProperty -- %s"), *CPPType);
	}
	else if (CastField<FInterfaceProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FInterfaceProperty -- %s"), *CPPType);
	}
	else if (CastField<FNameProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FNameProperty -- %s"), *CPPType);

		FNameProperty* NameProperty = CastField<FNameProperty>(Property);
		ValuePtr = NameProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = NameProperty->GetPropertyValue(ValuePtr).ToString();
	}
	else if (CastField<FStrProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FStrProperty -- %s"), *CPPType);

		FStrProperty* StrProperty = CastField<FStrProperty>(Property);
		ValuePtr = StrProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = StrProperty->GetPropertyValue(ValuePtr);
	}
	else if (CastField<FTextProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FTextProperty -- %s"), *CPPType);

		FTextProperty* TextProperty = CastField<FTextProperty>(Property);
		ValuePtr = TextProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = TextProperty->GetPropertyValue(ValuePtr).ToString();
	}
	else if (CastField<FArrayProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FArrayProperty -- %s"), *CPPType);

		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property);
		ValuePtr = ArrayProperty->ContainerPtrToValuePtr<uint8>(Object);
		ArrayProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
		KindType = (int32)EScriptUEKindType::T_TList;
	}
	else if (CastField<FSetProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FSetProperty -- %s"), *CPPType);

		FSetProperty* SetProperty = CastField<FSetProperty>(Property);
		ValuePtr = SetProperty->ContainerPtrToValuePtr<uint8>(Object);
		SetProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
		KindType = (int32)EScriptUEKindType::T_TList;
	}
	else if (CastField<FMapProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FMapProperty -- %s"), *CPPType);

		FMapProperty* MapProperty = CastField<FMapProperty>(Property);
		ValuePtr = MapProperty->ContainerPtrToValuePtr<uint8>(Object);
		MapProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
		KindType = (int32)EScriptUEKindType::T_TDict;
	}
	else if (CastField<FStructProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FStructProperty -- %s"), *CPPType);

		FStructProperty* StructProperty = CastField<FStructProperty>(Property);
		ValuePtr = StructProperty->ContainerPtrToValuePtr<uint8>(Object);
		StructProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);

		if (CPPType.Equals(FVectorName) || CPPType.Equals(FRotatorName))
			KindType = (int32)EScriptUEKindType::T_Single;
		else
			KindType = (int32)EScriptUEKindType::T_TDict;
	}
	else if (CastField<FDelegateProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UDelegateProperty -- %s"), *CPPType);
	}
	else if (CastField<FMulticastDelegateProperty>(Property))
	{
		VarType = FString::Printf(TEXT("FMulticastDelegateProperty -- %s"), *CPPType);
	}

	return true;
}

// UEObjectListen get Object Info by FClassDesc
void UScriptDebuggerSetting::UEObjectListen(FScriptDebuggerVarNode& InNode)
{
	int32 LoopIndex = 0;
	FString VarName;
	FString VarValue;
	FString VarType;
	int32 KindType;
	void* VarPtr = NULL;

	//for List and Dict to Generate Node
	TArray<FString> NameGroup;
	TArray<FString> ValueGroup;
	const TCHAR* ValueTravel;
	int32 ValueLen, PreStep, NowStep, BracketCount;

	switch (InNode.KindType)
	{
	case (int32)EScriptUEKindType::T_TList:

		ValueTravel = InNode.VarValue.ToString().GetCharArray().GetData();
		ValueLen = InNode.VarValue.ToString().Len();
		if (ValueLen < 3)
			return;

		PreStep = 1;
		NowStep = 1;
		BracketCount = 0;

		while (NowStep++ != ValueLen)
		{
			switch (*(ValueTravel + NowStep))
			{
			case TCHAR('('):
				BracketCount++;
				break;
			case TCHAR(')'):
				BracketCount--;
				break;
			case TCHAR(','):

				if (BracketCount == 0)
				{
					//splite the VarValue
					VarValue.Empty();
					VarValue.AppendChars(ValueTravel + PreStep, NowStep - PreStep);
					ValueGroup.Add(VarValue);
					PreStep = NowStep + 1;
				}

				break;
			}
		}

		//get the last value
		VarValue.Empty();
		VarValue.AppendChars(ValueTravel + PreStep, NowStep - PreStep - 2);
		ValueGroup.Add(VarValue);

		for (int i = 0; i < ValueGroup.Num(); ++i)
		{
			if (!InNode.NodeChildren.Contains(FString::FromInt(i)))
			{
				FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
				NewNode->NodeType = InNode.NodeType;
				NewNode->KindType = (int32)EScriptUEKindType::T_Single;
				NewNode->VarName = FText::FromString(FString::FromInt(i));
				NewNode->VarValue = FText::FromString(ValueGroup[i]);

				InNode.NodeChildren.Add(FString::FromInt(i), NewNode);
			}
		}


		break;
	case (int32)EScriptUEKindType::T_TDict:


		ValueTravel = InNode.VarValue.ToString().GetCharArray().GetData();
		ValueLen = InNode.VarValue.ToString().Len();
		if (ValueLen < 3)
			return;

		PreStep = 1;
		NowStep = 1;
		BracketCount = 0;

		while (NowStep++ != ValueLen)
		{
			switch (*(ValueTravel + NowStep))
			{
			case TCHAR('('):
				BracketCount++;
				break;
			case TCHAR(')'):
				BracketCount--;
				break;
			case TCHAR('='):
				if (BracketCount == 0)
				{
					//splite the VarName
					VarName.Empty();
					VarName.AppendChars(ValueTravel + PreStep, NowStep - PreStep);
					NameGroup.Add(VarName);
					PreStep = NowStep + 1;
				}
				break;
			case TCHAR(','):
				if (BracketCount == 0)
				{
					//splite the VarValue
					VarValue.Empty();
					VarValue.AppendChars(ValueTravel + PreStep, NowStep - PreStep);
					ValueGroup.Add(VarValue);
					PreStep = NowStep + 1;
				}
				break;
			}
		}

		//get the last value
		VarValue.Empty();
		VarValue.AppendChars(ValueTravel + PreStep, NowStep - PreStep - 2);
		ValueGroup.Add(VarValue);

		if (NameGroup.Num() == ValueGroup.Num())
		{
			for (int i = 0; i < NameGroup.Num(); ++i)
			{
				if (!InNode.NodeChildren.Contains(NameGroup[i]))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = InNode.NodeType;
					NewNode->KindType = (int32)EScriptUEKindType::T_Single;
					NewNode->VarName = FText::FromString(NameGroup[i]);
					NewNode->VarValue = FText::FromString(ValueGroup[i]);

					InNode.NodeChildren.Add(NameGroup[i], NewNode);
				}
			}
		}

		break;
	case (int32)EScriptUEKindType::T_UObject:
		if (InNode.VarPtr)
		{
			UObject* Object = (UObject*)InNode.VarPtr;

			//get property
			for (TFieldIterator<FProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				FProperty* Property = *ProIt;

				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, Object))
					{
						FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
						NewNode->NodeType = InNode.NodeType;
						NewNode->KindType = KindType;
						NewNode->VarName = FText::FromString(Property->GetNameCPP());
						NewNode->VarValue = FText::FromString(VarValue);
						NewNode->VarType = FText::FromString(VarType);
						NewNode->VarPtr = VarPtr;

						InNode.NodeChildren.Add(Property->GetNameCPP(), NewNode);
					}
				}
			}

#ifndef UNFOLD_FUNCTION

			//get function
			for (TFieldIterator<UFunction> FunIt(Object->GetClass()); FunIt; ++FunIt)
			{
				UFunction* Function = *FunIt;
				if (!InNode.NodeChildren.Contains(Function->GetName()))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = InNode.NodeType;
					NewNode->KindType = (int32)EScriptUEKindType::T_UFunction;
					NewNode->VarName = FText::FromString(Function->GetName());
					NewNode->VarValue = FText::FromString(Function->GetPathName());
					NewNode->VarType = UFunctionText;
					NewNode->VarPtr = (void*)Function;

					InNode.NodeChildren.Add(Function->GetName(), NewNode);
				}
			}

#endif


		}
		break;
	case (int32)EScriptUEKindType::T_AActor:
		if (InNode.VarPtr)
		{
			AActor* Object = (AActor*)InNode.VarPtr;

			//get transform at first
			FVector SelfLocation = Object->GetActorLocation();
			FRotator SelfRotator = Object->GetActorRotation();
			FVector SelfScaler = Object->GetActorScale3D();

			if (!InNode.NodeChildren.Contains(SelfLocationName))
			{
				FScriptDebuggerVarNode_Ref LocationNode = MakeShareable(new FScriptDebuggerVarNode);
				LocationNode->NodeType = InNode.NodeType;
				LocationNode->KindType = (int32)EScriptUEKindType::T_Single;
				LocationNode->VarName = SelfLocationText;
				LocationNode->VarValue = FText::FromString(SelfLocation.ToString());
				LocationNode->VarType = FVectorText;
				LocationNode->VarPtr = NULL;

				InNode.NodeChildren.Add(SelfLocationName, LocationNode);
			}

			if (!InNode.NodeChildren.Contains(SelfRotatorName))
			{
				FScriptDebuggerVarNode_Ref RotatorNode = MakeShareable(new FScriptDebuggerVarNode);
				RotatorNode->NodeType = InNode.NodeType;
				RotatorNode->KindType = (int32)EScriptUEKindType::T_Single;
				RotatorNode->VarName = SelfRotatorText;
				RotatorNode->VarValue = FText::FromString(SelfRotator.ToString());
				RotatorNode->VarType = FRotatorText;
				RotatorNode->VarPtr = NULL;

				InNode.NodeChildren.Add(SelfRotatorName, RotatorNode);
			}

			if (!InNode.NodeChildren.Contains(SelfScalerName))
			{
				FScriptDebuggerVarNode_Ref ScalerNode = MakeShareable(new FScriptDebuggerVarNode);
				ScalerNode->NodeType = InNode.NodeType;
				ScalerNode->KindType = (int32)EScriptUEKindType::T_Single;
				ScalerNode->VarName = SelfScalerText;
				ScalerNode->VarValue = FText::FromString(SelfScaler.ToString());
				ScalerNode->VarType = FVectorText;
				ScalerNode->VarPtr = NULL;

				InNode.NodeChildren.Add(SelfScalerName, ScalerNode);
			}

			//get property
			for (TFieldIterator<FProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				FProperty* Property = *ProIt;

				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, Object))
					{
						FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
						NewNode->NodeType = InNode.NodeType;
						NewNode->KindType = KindType;
						NewNode->VarName = FText::FromString(Property->GetNameCPP());
						NewNode->VarValue = FText::FromString(VarValue);
						NewNode->VarType = FText::FromString(VarType);
						NewNode->VarPtr = VarPtr;

						InNode.NodeChildren.Add(Property->GetNameCPP(), NewNode);
					}
				}
			}

#ifndef UNFOLD_FUNCTION

			//get function
			for (TFieldIterator<UFunction> FunIt(Object->GetClass()); FunIt; ++FunIt)
			{
				UFunction* Function = *FunIt;
				if (!InNode.NodeChildren.Contains(Function->GetName()))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = InNode.NodeType;
					NewNode->KindType = (int32)EScriptUEKindType::T_UFunction;
					NewNode->VarName = FText::FromString(Function->GetName());
					NewNode->VarValue = FText::FromString(Function->GetPathName());
					NewNode->VarType = UFunctionText;
					NewNode->VarPtr = (void*)Function;

					InNode.NodeChildren.Add(Function->GetName(), NewNode);
				}
			}
#endif
		}
		break;
	case (int32)EScriptUEKindType::T_UEObject:
		if (InNode.VarPtr)
		{
			//if UEClassDesc exit, Create ClassDesc at First
			if (UEClassDesc && !InNode.NodeChildren.Contains(UEClassDesc->GetName()))
			{
				FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
				NewNode->NodeType = InNode.NodeType;
				NewNode->KindType = (int32)EScriptUEKindType::T_ClassDesc;
				NewNode->VarName = FText::FromString(UEClassDesc->GetName());
				NewNode->VarType = ClassDescText;
				NewNode->VarPtr = (void*)UEClassDesc;

				InNode.NodeChildren.Add(UEClassDesc->GetName(), NewNode);
			}

			//get transform
			AActor* UEActor = Cast<AActor>(UEObject);
			if (UEActor)
			{
				//get transform at first
				FVector SelfLocation = UEActor->GetActorLocation();
				FRotator SelfRotator = UEActor->GetActorRotation();
				FVector SelfScaler = UEActor->GetActorScale3D();

				if (!InNode.NodeChildren.Contains(SelfLocationName))
				{
					FScriptDebuggerVarNode_Ref LocationNode = MakeShareable(new FScriptDebuggerVarNode);
					LocationNode->NodeType = InNode.NodeType;
					LocationNode->KindType = (int32)EScriptUEKindType::T_Single;
					LocationNode->VarName = SelfLocationText;
					LocationNode->VarValue = FText::FromString(SelfLocation.ToString());
					LocationNode->VarType = FVectorText;
					LocationNode->VarPtr = NULL;

					InNode.NodeChildren.Add(SelfLocationName, LocationNode);
				}

				if (!InNode.NodeChildren.Contains(SelfRotatorName))
				{
					FScriptDebuggerVarNode_Ref RotatorNode = MakeShareable(new FScriptDebuggerVarNode);
					RotatorNode->NodeType = InNode.NodeType;
					RotatorNode->KindType = (int32)EScriptUEKindType::T_Single;
					RotatorNode->VarName = SelfRotatorText;
					RotatorNode->VarValue = FText::FromString(SelfRotator.ToString());
					RotatorNode->VarType = FRotatorText;
					RotatorNode->VarPtr = NULL;

					InNode.NodeChildren.Add(SelfRotatorName, RotatorNode);
				}

				if (!InNode.NodeChildren.Contains(SelfScalerName))
				{
					FScriptDebuggerVarNode_Ref ScalerNode = MakeShareable(new FScriptDebuggerVarNode);
					ScalerNode->NodeType = InNode.NodeType;
					ScalerNode->KindType = (int32)EScriptUEKindType::T_Single;
					ScalerNode->VarName = SelfScalerText;
					ScalerNode->VarValue = FText::FromString(SelfScaler.ToString());
					ScalerNode->VarType = FVectorText;
					ScalerNode->VarPtr = NULL;

					InNode.NodeChildren.Add(SelfScalerName, ScalerNode);
				}
			}

			//get property
			for (TFieldIterator<FProperty> ProIt(UEObject->GetClass()); ProIt; ++ProIt)
			{
				FProperty* Property = *ProIt;

				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, UEObject))
					{
						FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
						NewNode->NodeType = InNode.NodeType;
						NewNode->KindType = KindType;
						NewNode->VarName = FText::FromString(Property->GetNameCPP());
						NewNode->VarValue = FText::FromString(VarValue);
						NewNode->VarType = FText::FromString(VarType);
						NewNode->VarPtr = VarPtr;

						InNode.NodeChildren.Add(Property->GetNameCPP(), NewNode);
					}
				}
			}

#ifndef UNFOLD_FUNCTION

			//get function
			for (TFieldIterator<UFunction> FunIt(UEObject->GetClass()); FunIt; ++FunIt)
			{
				UFunction* Function = *FunIt;
				if (!InNode.NodeChildren.Contains(Function->GetName()))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = InNode.NodeType;
					NewNode->KindType = (int32)EScriptUEKindType::T_UFunction;
					NewNode->VarName = FText::FromString(Function->GetName());
					NewNode->VarValue = FText::FromString(Function->GetPathName());
					NewNode->VarType = UFunctionText;
					NewNode->VarPtr = (void*)Function;

					InNode.NodeChildren.Add(Function->GetName(), NewNode);
				}
			}
#endif
		}
		break;
	case (int32)EScriptUEKindType::T_ClassDesc:
		if (InNode.VarPtr)
		{
			FClassDesc* ClassDesc = (FClassDesc*)InNode.VarPtr;

			//get parent classdesc
			if (ClassDesc->GetParent() && !InNode.NodeChildren.Contains(ClassDesc->GetParent()->GetName()))
			{
				FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
				NewNode->NodeType = InNode.NodeType;
				NewNode->KindType = (int32)EScriptUEKindType::T_ClassDesc;
				NewNode->VarName = FText::FromString(ClassDesc->GetParent()->GetName());
				NewNode->VarType = ClassDescText;
				NewNode->VarPtr = (void*)ClassDesc->GetParent();

				InNode.NodeChildren.Add(ClassDesc->GetParent()->GetName(), NewNode);
			}

			//get property desc
			FPropertyDesc* PropertyDesc = ClassDesc->GetProperty(LoopIndex);
			while (PropertyDesc)
			{
				FProperty* Property = PropertyDesc->GetProperty();
				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					//T_ClassDesc use the UEObject to find Property
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, UEObject))
					{
						FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
						NewNode->NodeType = InNode.NodeType;
						NewNode->KindType = KindType;
						NewNode->VarName = FText::FromString(Property->GetNameCPP());
						NewNode->VarValue = FText::FromString(VarValue);
						NewNode->VarType = FText::FromString(VarType);
						NewNode->VarPtr = VarPtr;

						InNode.NodeChildren.Add(Property->GetNameCPP(), NewNode);
					}
				}
				PropertyDesc = ClassDesc->GetProperty(++LoopIndex);
			}

#ifndef UNFOLD_FUNCTION
			//get function desc
			LoopIndex = 0;
			FFunctionDesc* FunctionDesc = ClassDesc->GetFunction(LoopIndex);
			while (FunctionDesc)
			{
				UFunction* Function = FunctionDesc->GetFunction();
				if (!InNode.NodeChildren.Contains(Function->GetName()))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = InNode.NodeType;
					NewNode->KindType = (int32)EScriptUEKindType::T_UFunction;
					NewNode->VarName = FText::FromString(Function->GetName());
					NewNode->VarValue = FText::FromString(Function->GetPathName());
					NewNode->VarType = UFunctionText;
					NewNode->VarPtr = (void*)Function;

					InNode.NodeChildren.Add(Function->GetName(), NewNode);
				}
				FunctionDesc = ClassDesc->GetFunction(++LoopIndex);
			}
#endif

		}
		break;
	case (int32)EScriptUEKindType::T_UFunction:

		if (InNode.VarPtr)
		{
			UFunction* Function = (UFunction*)InNode.VarPtr;

			//iter function var
			for (TFieldIterator<FProperty> ProIt(Function); ProIt; ++ProIt)
			{
				FProperty* Property = *ProIt;

				VarName = Property->GetNameCPP();
				if (!VarName.Equals(ReturnValueName) && !InNode.NodeChildren.Contains(VarName))
				{
					FScriptDebuggerVarNode_Ref NewNode = MakeShareable(new FScriptDebuggerVarNode);
					NewNode->NodeType = InNode.NodeType;
					NewNode->KindType = (int32)EScriptUEKindType::T_Single;
					NewNode->VarName = FText::FromString(VarName);
					NewNode->VarType = FText::FromString(Property->GetCPPType());

					InNode.NodeChildren.Add(Property->GetNameCPP(), NewNode);
				}

			}
		}
		break;
	}
}

void UScriptDebuggerSetting::BindDebugState()
{
	if (!RegLuaHandle.IsValid())
		RegLuaHandle = FUnLuaDelegates::OnLuaStateCreated.AddUObject(this, &UScriptDebuggerSetting::RegisterLuaState);

	if (!UnRegLuaHandle.IsValid())
		UnRegLuaHandle = FUnLuaDelegates::OnPreLuaContextCleanup.AddUObject(this, &UScriptDebuggerSetting::UnRegisterLuaState);
}

void UScriptDebuggerSetting::UnBindDebugState()
{
	if (RegLuaHandle.IsValid())
	{
		FUnLuaDelegates::OnLuaStateCreated.Remove(RegLuaHandle);
		RegLuaHandle.Reset();
	}

	if (UnRegLuaHandle.IsValid())
	{
		FUnLuaDelegates::OnPreLuaContextCleanup.Remove(UnRegLuaHandle);
		UnRegLuaHandle.Reset();
	}

	if (L)
	{
		lua_sethook(L, NULL, 0, 0);
		L = NULL;
	}
}

void UScriptDebuggerSetting::OnObjectBinded(UObjectBaseUtility* InObject)
{
	FString FuncPath;
	FString FuncName;
	FString ClassName;
	FString TempStr;

	for (TFieldIterator<UFunction> FunIt(InObject->GetClass()); FunIt; ++FunIt)
	{
		UFunction* Function = *FunIt;

		Function->GetPathName().Split(":", &FuncPath, &FuncName);

		if (ScriptPromptGroup.FindOrAdd(ClassName).Contains(FuncName))
		{
			continue;
		}

		FuncPath.Split(".", &TempStr, &ClassName);

		FString ProName;
		FString ProType;

		FString CodeClip = FString::Printf(TEXT("%s("), *FuncName);
		FString ToolTip = FString::Printf(TEXT("(*%s)("), *FuncName);

		bool HasReturn = false;

		//iter function var
		for (TFieldIterator<FProperty> ProIt(Function); ProIt; ++ProIt)
		{
			FProperty* Property = *ProIt;

			ProName = Property->GetNameCPP();
			ProType = Property->GetCPPType();

			if (!ProName.Equals(ReturnValueName))
			{
				CodeClip += FString::Printf(TEXT("%s, "), *ProName);
				ToolTip += FString::Printf(TEXT("%s, "), *ProType);
			}
			else
			{
				ToolTip = FString::Printf(TEXT("%s%s"), *ProType, *ToolTip);
				HasReturn = true;
			}
		}

		CodeClip.RemoveFromEnd(TEXT(", "));
		CodeClip += TEXT(")");

		ToolTip.RemoveFromEnd(TEXT(", "));
		ToolTip += TEXT(")");

		if (!HasReturn)
			ToolTip = FString::Printf(TEXT("void%s"), *ToolTip);

		ScriptPromptGroup.FindOrAdd(ClassName).Add(FuncName, FScriptPromptNode(ClassName, FuncName, ToolTip, CodeClip));
	}
}

void UScriptDebuggerSetting::EnterDebug(const FString& LuaFilePath, int32 Line)
{

	//Collect Stack Info
	TArray<TTuple<int32, int32, FString, FString>> StackInfos;
	int i = 0;
	lua_Debug ar;
	while (lua_getstack(L, i, &ar) != 0 && i < 10)
	{
		if (lua_getinfo(L, "Snl", &ar) != 0)
		{
			TTuple<int32, int32, FString, FString> StackItem(i, ar.currentline, UTF8_TO_TCHAR(++ar.source), UTF8_TO_TCHAR(ar.name));
			StackInfos.Add(StackItem);
		}
		i++;
	}
	SScriptDebugger::Get()->SetStackData(StackInfos);

	SScriptDebugger::Get()->EnterDebug(LuaFilePath, Line);
}

void UScriptDebuggerSetting::GetStackVars(int32 StackIndex, TArray<FScriptDebuggerVarNode_Ref>& Vars)
{

	FScriptDebuggerVarNode_Ref LocalNode = MakeShareable(new FScriptDebuggerVarNode);
	LocalNode->NodeType = EScriptVarNodeType::V_Local;
	LocalNode->StackLevel = StackIndex;
	LocalNode->VarName = FText::FromString("Local");

	FScriptDebuggerVarNode_Ref UpValueNode = MakeShareable(new FScriptDebuggerVarNode);
	UpValueNode->NodeType = EScriptVarNodeType::V_UpValue;
	UpValueNode->StackLevel = StackIndex;
	UpValueNode->VarName = FText::FromString("UpValue");

	FScriptDebuggerVarNode_Ref GlobalNode = MakeShareable(new FScriptDebuggerVarNode);
	GlobalNode->NodeType = EScriptVarNodeType::V_Global;
	GlobalNode->StackLevel = StackIndex;
	GlobalNode->VarName = FText::FromString("Global");

	Vars.Add(LocalNode);
	Vars.Add(UpValueNode);
	Vars.Add(GlobalNode);

	// try to get ueobject
	lua_Debug ar;
	if (lua_getstack(L, StackIndex, &ar) != 0)
	{
		int i = 1;
		const char* VarName;
		int32 KindType = -1;
		while ((VarName = lua_getlocal(L, &ar, i)) != NULL)
		{
			// if get the self table
			if (SelfName.Equals(UTF8_TO_TCHAR(VarName)))
			{
				KindType = lua_type(L, -1);
				if (KindType == LUA_TTABLE)
				{
					lua_pushstring(L, "Object");
					lua_rawget(L, -2);

					if (lua_isuserdata(L, -1))
					{
						void* Userdata = lua_touserdata(L, -1);

						UEObject = (UObject*)*((void**)Userdata);

						if (UEObject)
						{

							// get matetable form userdate
							lua_getmetatable(L, -1);

							lua_pushstring(L, "ClassDesc");
							lua_rawget(L, -2);
							KindType = lua_type(L, -1);

							//makesure ClassDesc is lightuserdata
							if (lua_islightuserdata(L, -1))
							{
								void* UserData = lua_touserdata(L, -1);
								UEClassDesc = (FClassDesc*)UserData;

								if (UEClassDesc)
								{
									FScriptDebuggerVarNode_Ref UEObjectNode = MakeShareable(new FScriptDebuggerVarNode);
									UEObjectNode->NodeType = EScriptVarNodeType::V_UEObject;
									UEObjectNode->KindType = (int32)EScriptUEKindType::T_UEObject;
									UEObjectNode->VarPtr = (void*)UEObject;
									UEObjectNode->VarName = FText::FromString(UEObject->GetName());

									Vars.Add(UEObjectNode);
								}
							}
							lua_pop(L, 2);

						}
					}
					lua_pop(L, 1);
				}
				lua_pop(L, 1);
				break;
			}
			lua_pop(L, 1);
			i++;
		}
	}
}

void UScriptDebuggerSetting::GetVarsChildren(FScriptDebuggerVarNode& InNode)
{
	//UE_LOG(LogTemp, Log, TEXT("GetVarsChildren luagettop[%d]"), lua_gettop(L));

	//UEObject NameList is not empty
	if (InNode.NodeType == EScriptVarNodeType::V_UEObject)
	{
		UEObjectListen(InNode);
		return;
	}

	if (InNode.NameList.Num() > 0 && InNode.KindType != LUA_TTABLE)
		return;

	switch (InNode.NodeType)
	{
	case EScriptVarNodeType::V_Local:
		LocalListen(InNode);
		break;
	case EScriptVarNodeType::V_UpValue:
		UpvalueListen(InNode);
		break;
	case EScriptVarNodeType::V_Global:
		GlobalListen(InNode);
		break;
	}
}

void UScriptDebuggerSetting::Continue()
{
	if (L == NULL)
		return;

	hook_mode = EHookMode::H_Continue;
}

void UScriptDebuggerSetting::StepOver()
{
	if (L == NULL)
		return;

	u_over.Reset();
	hook_mode = EHookMode::H_StepOver;

}

void UScriptDebuggerSetting::StepIn()
{
	if (L == NULL)
		return;
	hook_mode = EHookMode::H_StepIn;
}

void UScriptDebuggerSetting::StepOut()
{
	if (L == NULL)
		return;

	u_out.Reset();
	hook_mode = EHookMode::H_StepOut;

}

void UScriptDebuggerSetting::hook_call_option()
{
	switch (hook_mode)
	{
	case EHookMode::H_Continue:
		break;
	case EHookMode::H_StepIn:
		break;
	case EHookMode::H_StepOver:
		u_over.CallOper();
		break;
	case EHookMode::H_StepOut:
		u_out.CallOper();
		break;
	}
}

void UScriptDebuggerSetting::hook_ret_option()
{
	switch (hook_mode)
	{
	case EHookMode::H_Continue:
		break;
	case EHookMode::H_StepIn:
		break;
	case EHookMode::H_StepOver:
		u_over.RetOper();
		break;
	case EHookMode::H_StepOut:
		u_out.RetOper();
		break;
	}
}

void UScriptDebuggerSetting::hook_line_option()
{
	switch (hook_mode)
	{
	case EHookMode::H_Continue:
		if (FScriptEditor::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			EnterDebug(ur.source, ur.currentline);
		}
		break;
	case EHookMode::H_StepIn:
		EnterDebug(ur.source, ur.currentline);
		break;
	case EHookMode::H_StepOver:
		if (u_over.BreakPoint())
		{
			EnterDebug(ur.source, ur.currentline);
			return;
		}
		if (FScriptEditor::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			EnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	case EHookMode::H_StepOut:
		if (u_out.BreakPoint())
		{
			EnterDebug(ur.source, ur.currentline);
			return;
		}
		if (FScriptEditor::Get()->HasBreakPoint(ur.source, ur.currentline))
		{
			EnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	}
}

static void debugger_hook_c(lua_State *L, lua_Debug *ar)
{

	if (lua_getinfo(L, "Snl", ar) != 0)
	{

#ifdef UNLUA_DEBUG
		ur.Init(ar);
		UE_LOG(LogTemp, Log, TEXT("%s"), *ur.ToString());
		if (ar->currentline < 0)
			return;
#else
		if (ar->currentline < 0)
			return;

		ur.Init(ar);
#endif // UNLUA_DEBUG

		switch (ar->event)
		{
		case LUA_HOOKCALL:
			UScriptDebuggerSetting::Get()->hook_call_option();
			break;
		case LUA_HOOKRET:
			UScriptDebuggerSetting::Get()->hook_ret_option();
			break;
		case LUA_HOOKLINE:
			UScriptDebuggerSetting::Get()->hook_line_option();
			break;
		}
	}
}

#undef UNLUA_DEBUG
#undef UNFOLD_FUNCTION


