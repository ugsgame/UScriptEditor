// Fill out your copyright notice in the Description page of Project Settings.

#include "VarNode.h"
#include "VarWatcher.h"
#include "UnLuaDelegates.h"
#include "lua.hpp"
#include "UnLua.h"
#include "UEReflectionUtils.h"
#include "GameFramework/Actor.h"

void FVarWatcherNode::GetChildren(TArray<TSharedRef<FVarWatcherNode>>& OutChildren)
{
	UVarWatcherSetting::Get()->GetVarChildren(*this);

	NodeChildren.GenerateValueArray(OutChildren);
}

const FString UVarWatcherSetting::TempVarName(TEXT("(*temporary)"));

const FText UVarWatcherSetting::UFunctionText = FText::FromString(TEXT("UFunction"));

const FText UVarWatcherSetting::ClassDescText = FText::FromString(TEXT("ClassDesc"));

const FString UVarWatcherSetting::SelfLocationName(TEXT("SelfLocation"));

const FString UVarWatcherSetting::SelfRotatorName(TEXT("SelfRotator"));

const FString UVarWatcherSetting::SelfScalerName(TEXT("SelfScaler"));

const FString UVarWatcherSetting::FVectorName(TEXT("FVector"));

const FString UVarWatcherSetting::FRotatorName(TEXT("FRotator"));

const FString UVarWatcherSetting::ReturnValueName(TEXT("ReturnValue"));

const FText UVarWatcherSetting::SelfLocationText = FText::FromString(SelfLocationName);

const FText UVarWatcherSetting::SelfRotatorText = FText::FromString(SelfRotatorName);

const FText UVarWatcherSetting::SelfScalerText = FText::FromString(SelfScalerName);

const FText UVarWatcherSetting::FVectorText = FText::FromString("FVector");

const FText UVarWatcherSetting::FRotatorText = FText::FromString("FRotator");

UVarWatcherSetting* UVarWatcherSetting::Get()
{
	static UVarWatcherSetting* Singleton = NULL;
	if (Singleton == NULL)
	{
		Singleton = NewObject<UVarWatcherSetting>();
		Singleton->AddToRoot();

	}
	return Singleton;
}

void UVarWatcherSetting::SetTapIsOpen(bool IsOpen)
{
	IsTapOpen = IsOpen;

	if (IsTapOpen)
	{
		RegLuaHandle = FUnLuaDelegates::OnLuaStateCreated.AddUObject(this, &UVarWatcherSetting::RegisterLuaState);

		UnRegLuaHandle = FUnLuaDelegates::OnPostLuaContextCleanup.AddUObject(this, &UVarWatcherSetting::UnRegisterLuaState);

		BindObjectHandle = FUnLuaDelegates::OnObjectBinded.AddUObject(this, &UVarWatcherSetting::OnObjectBinded);

		UnBindObjectHandle = FUnLuaDelegates::OnObjectUnbinded.AddUObject(this, &UVarWatcherSetting::OnObjectUnbinded);

		//UE_LOG(LogTemp, Log, TEXT("UVarWatcherSetting::SetTapIsOpen open"));
	}
	else
	{
		FUnLuaDelegates::OnLuaStateCreated.Remove(RegLuaHandle);

		FUnLuaDelegates::OnPostLuaContextCleanup.Remove(UnRegLuaHandle);

		FUnLuaDelegates::OnObjectBinded.Remove(BindObjectHandle);

		FUnLuaDelegates::OnObjectUnbinded.Remove(UnBindObjectHandle);

		VarTreeRoot.Reset();

		SVarWatcher::Get()->RefreshVarTree();

		//UE_LOG(LogTemp, Log, TEXT("UVarWatcherSetting::SetTapIsOpen close"));
	}
}

void UVarWatcherSetting::RegisterLuaState(lua_State* State)
{

	L = State;

	//add global node
	FVarWatcherNode_Ref GlobalNode = MakeShareable(new FVarWatcherNode);
	GlobalNode->NodeType = EScriptVarNodeType::Global;
	GlobalNode->VarName = FText::FromString("Global");

	VarTreeRoot.Add(GlobalNode);
	SVarWatcher::Get()->RefreshVarTree();
}

void UVarWatcherSetting::UnRegisterLuaState(bool bFullCleanup)
{

	L = NULL;

	VarTreeRoot.Reset();
	SVarWatcher::Get()->RefreshVarTree();
}

void UVarWatcherSetting::OnObjectBinded(UObjectBaseUtility* InObject)
{

	ObjectGroup.Add(InObject);

	//add this Object
	FVarWatcherNode_Ref ObjectNode = MakeShareable(new FVarWatcherNode);
	ObjectNode->NodeType = EScriptVarNodeType::UEObject;
	ObjectNode->KindType = (int32)EScriptUEKindType::T_UEObject;
	ObjectNode->VarName = FText::FromString(InObject->GetName());
	ObjectNode->VarPtr = (void*)InObject;

	VarTreeRoot.Add(ObjectNode);
	SVarWatcher::Get()->RefreshVarTree();
}

void UVarWatcherSetting::OnObjectUnbinded(UObjectBaseUtility* InObject)
{
	
	ObjectGroup.Remove(InObject);

	for (auto Node : VarTreeRoot)
	{
		if (Node->VarPtr == (void*)InObject)
		{
			VarTreeRoot.Remove(Node);
			break;
		}
	}

	SVarWatcher::Get()->RefreshVarTree();
}

void UVarWatcherSetting::Update(float DeltaTime)
{
	for (auto Node : VarTreeRoot)
	{
		if (Node->NodeType == EScriptVarNodeType::UEObject)
		{
			if (Node->NodeChildren.Contains(SelfLocationName))
			{
				AActor* UEActor = Cast<AActor>((UObject*)Node->VarPtr);

				FVector SelfLocation = UEActor->GetActorLocation();
				FRotator SelfRotator = UEActor->GetActorRotation();
				FVector SelfScaler = UEActor->GetActorScale3D();

				(*Node->NodeChildren.Find(SelfLocationName))->VarValue = FText::FromString(SelfLocation.ToString());

				(*Node->NodeChildren.Find(SelfRotatorName))->VarValue = FText::FromString(SelfRotator.ToString());

				(*Node->NodeChildren.Find(SelfScalerName))->VarValue = FText::FromString(SelfScaler.ToString());
			}
		}
	}
}

void UVarWatcherSetting::IteraionTable(FVarWatcherNode& InNode, int32 NameIndex)
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

						FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
						NewNode->NodeType = InNode.NodeType;
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

bool UVarWatcherSetting::NameTranslate(int32 KindType, FString& VarName, int32 StackIndex)
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

void UVarWatcherSetting::ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex)
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

void UVarWatcherSetting::GlobalListen(FVarWatcherNode& InNode)
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
				FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
				NewNode->NodeType = EScriptVarNodeType::Global;
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

bool UVarWatcherSetting::PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, UProperty* Property, UObject* Object)
{
	//reset data
	VarValue.Empty();
	KindType = (int32)EScriptUEKindType::T_Single;
	VarPtr = NULL;

	FString CPPType = Property->GetCPPType();
	void* ValuePtr;

	if (Cast<UNumericProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UNumericProperty -- %s"), *CPPType);

		UNumericProperty* NumericProperty = Cast<UNumericProperty>(Property);
		ValuePtr = NumericProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = NumericProperty->GetNumericPropertyValueToString(ValuePtr);
	}
	else if (Cast<UEnumProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UEnumProperty -- %s"), *CPPType);

		UEnumProperty* EnumProperty = Cast<UEnumProperty>(Property);
		ValuePtr = EnumProperty->ContainerPtrToValuePtr<uint8>(Object);
		EnumProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
	}
	else if (Cast<UBoolProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UBoolProperty -- %s"), *CPPType);

		UBoolProperty* BoolProperty = Cast<UBoolProperty>(Property);
		ValuePtr = BoolProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = BoolProperty->GetPropertyValue(ValuePtr) ? TEXT("True") : TEXT("False");
	}
	else if (Cast<UObjectPropertyBase>(Property))
	{
		VarType = FString::Printf(TEXT("UObjectPropertyBase -- %s"), *CPPType);

		UObjectPropertyBase* ObjectPropertyBase = Cast<UObjectPropertyBase>(Property);
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
	else if (Cast<USoftObjectProperty>(Property))
	{
		VarType = FString::Printf(TEXT("USoftObjectProperty -- %s"), *CPPType);
	}
	else if (Cast<UInterfaceProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UInterfaceProperty -- %s"), *CPPType);
	}
	else if (Cast<UNameProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UNameProperty -- %s"), *CPPType);

		UNameProperty* NameProperty = Cast<UNameProperty>(Property);
		ValuePtr = NameProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = NameProperty->GetPropertyValue(ValuePtr).ToString();
	}
	else if (Cast<UStrProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UStrProperty -- %s"), *CPPType);

		UStrProperty* StrProperty = Cast<UStrProperty>(Property);
		ValuePtr = StrProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = StrProperty->GetPropertyValue(ValuePtr);
	}
	else if (Cast<UTextProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UTextProperty -- %s"), *CPPType);

		UTextProperty* TextProperty = Cast<UTextProperty>(Property);
		ValuePtr = TextProperty->ContainerPtrToValuePtr<uint8>(Object);
		VarValue = TextProperty->GetPropertyValue(ValuePtr).ToString();
	}
	else if (Cast<UArrayProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UArrayProperty -- %s"), *CPPType);

		UArrayProperty* ArrayProperty = Cast<UArrayProperty>(Property);
		ValuePtr = ArrayProperty->ContainerPtrToValuePtr<uint8>(Object);
		ArrayProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
		KindType = (int32)EScriptUEKindType::T_TList;
	}
	else if (Cast<USetProperty>(Property))
	{
		VarType = FString::Printf(TEXT("USetProperty -- %s"), *CPPType);

		USetProperty* SetProperty = Cast<USetProperty>(Property);
		ValuePtr = SetProperty->ContainerPtrToValuePtr<uint8>(Object);
		SetProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
		KindType = (int32)EScriptUEKindType::T_TList;
	}
	else if (Cast<UMapProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UMapProperty -- %s"), *CPPType);

		UMapProperty* MapProperty = Cast<UMapProperty>(Property);
		ValuePtr = MapProperty->ContainerPtrToValuePtr<uint8>(Object);
		MapProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);
		KindType = (int32)EScriptUEKindType::T_TDict;
	}
	else if (Cast<UStructProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UStructProperty -- %s"), *CPPType);

		UStructProperty* StructProperty = Cast<UStructProperty>(Property);
		ValuePtr = StructProperty->ContainerPtrToValuePtr<uint8>(Object);
		StructProperty->ExportTextItem(VarValue, ValuePtr, ValuePtr, Object, 0, NULL);

		if (CPPType.Equals(FVectorName) || CPPType.Equals(FRotatorName))
			KindType = (int32)EScriptUEKindType::T_Single;
		else
			KindType = (int32)EScriptUEKindType::T_TDict;
	}
	else if (Cast<UDelegateProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UDelegateProperty -- %s"), *CPPType);
	}
	else if (Cast<UMulticastDelegateProperty>(Property))
	{
		VarType = FString::Printf(TEXT("UMulticastDelegateProperty -- %s"), *CPPType);
	}

	return true;
}

void UVarWatcherSetting::UEObjectListen(FVarWatcherNode& InNode)
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
				FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
					FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
			for (TFieldIterator<UProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, Object))
					{
						FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
					FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
				FVarWatcherNode_Ref LocationNode = MakeShareable(new FVarWatcherNode);
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
				FVarWatcherNode_Ref RotatorNode = MakeShareable(new FVarWatcherNode);
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
				FVarWatcherNode_Ref ScalerNode = MakeShareable(new FVarWatcherNode);
				ScalerNode->NodeType = InNode.NodeType;
				ScalerNode->KindType = (int32)EScriptUEKindType::T_Single;
				ScalerNode->VarName = SelfScalerText;
				ScalerNode->VarValue = FText::FromString(SelfScaler.ToString());
				ScalerNode->VarType = FVectorText;
				ScalerNode->VarPtr = NULL;

				InNode.NodeChildren.Add(SelfScalerName, ScalerNode);
			}

			//get property
			for (TFieldIterator<UProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, Object))
					{
						FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
					FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
			UObject* Object = (UObject*)InNode.VarPtr;
			//get transform
			AActor* UEActor = Cast<AActor>(Object);
			if (UEActor)
			{
				//get transform at first
				FVector SelfLocation = UEActor->GetActorLocation();
				FRotator SelfRotator = UEActor->GetActorRotation();
				FVector SelfScaler = UEActor->GetActorScale3D();

				if (!InNode.NodeChildren.Contains(SelfLocationName))
				{
					FVarWatcherNode_Ref LocationNode = MakeShareable(new FVarWatcherNode);
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
					FVarWatcherNode_Ref RotatorNode = MakeShareable(new FVarWatcherNode);
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
					FVarWatcherNode_Ref ScalerNode = MakeShareable(new FVarWatcherNode);
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
			for (TFieldIterator<UProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

				if (!InNode.NodeChildren.Contains(Property->GetNameCPP()))
				{
					if (PropertyTranslate(VarValue, VarType, KindType, VarPtr, Property, Object))
					{
						FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
					FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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
	case (int32)EScriptUEKindType::T_UFunction:

		if (InNode.VarPtr)
		{
			UFunction* Function = (UFunction*)InNode.VarPtr;

			//iter function var
			for (TFieldIterator<UProperty> ProIt(Function); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

				VarName = Property->GetNameCPP();
				if (!VarName.Equals(ReturnValueName) && !InNode.NodeChildren.Contains(VarName))
				{
					FVarWatcherNode_Ref NewNode = MakeShareable(new FVarWatcherNode);
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

void UVarWatcherSetting::GetVarChildren(FVarWatcherNode& InNode)
{
	switch (InNode.NodeType)
	{
	case EScriptVarNodeType::UEObject:
		UEObjectListen(InNode);
		break;
	case EScriptVarNodeType::Global:
		GlobalListen(InNode);
		break;
	}
}
