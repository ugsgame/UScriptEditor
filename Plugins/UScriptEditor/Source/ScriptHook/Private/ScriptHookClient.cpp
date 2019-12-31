// Fill out your copyright notice in the Description page of Project Settings.

#include "ScriptHookClient.h"
#include "UnLua.h"
#include "UnLuaDelegates.h"
#include "SocketSubsystem.h"
#include "Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "Sockets.h"
#include "ScriptHookReceive.h"
#include "ScriptHookDeal.h"
#include "ScriptBreakPoints.h"
#include "ScriptDebugInfo.h"
#include "ScriptReqStack.h"
#include "ScriptStackVars.h"
#include "ScriptReqChild.h"
#include "ScriptChildVars.h"
#include "SlateApplication.h"
#include "GameFramework/Actor.h"
#include "LuaContext.h"
#include "UEReflectionUtils.h"

static void debugger_hook_c(lua_State *L, lua_Debug *ar);

const FString UScriptHookClient::TempVarName(TEXT("(*temporary)"));

const FString UScriptHookClient::SelfName(TEXT("self"));

const FString UScriptHookClient::OverriddenName(TEXT("Overridden"));

const FText UScriptHookClient::UFunctionText = FText::FromString(TEXT("UFunction"));

const FText UScriptHookClient::ClassDescText = FText::FromString(TEXT("ClassDesc"));

const FString UScriptHookClient::SelfLocationName(TEXT("SelfLocation"));

const FString UScriptHookClient::SelfRotatorName(TEXT("SelfRotator"));

const FString UScriptHookClient::SelfScalerName(TEXT("SelfScaler"));

const FString UScriptHookClient::FVectorName(TEXT("FVector"));

const FString UScriptHookClient::FRotatorName(TEXT("FRotator"));

const FString UScriptHookClient::ReturnValueName(TEXT("ReturnValue"));

const FText UScriptHookClient::SelfLocationText = FText::FromString(SelfLocationName);

const FText UScriptHookClient::SelfRotatorText = FText::FromString(SelfRotatorName);

const FText UScriptHookClient::SelfScalerText = FText::FromString(SelfScalerName);

const FText UScriptHookClient::FVectorText = FText::FromString("FVector");

const FText UScriptHookClient::FRotatorText = FText::FromString("FRotator");

const FString UScriptHookClient::ScriptMask(TEXT("/Script/"));

UScriptHookClient* UScriptHookClient::Get()
{
	static UScriptHookClient* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptHookClient>();
		Singleton->AddToRoot();

		Singleton->BindDebugState();

	}
	return Singleton;
}

void UScriptHookClient::BindDebugState()
{
	if (!RegLuaHandle.IsValid())
		RegLuaHandle = FUnLuaDelegates::OnLuaStateCreated.AddUObject(this, &UScriptHookClient::RegisterLuaState);

	if (!UnRegLuaHandle.IsValid())
		UnRegLuaHandle = FUnLuaDelegates::OnPostLuaContextCleanup.AddUObject(this, &UScriptHookClient::UnRegisterLuaState);
}

void UScriptHookClient::UnBindDebugState()
{
	if (RegLuaHandle.IsValid())
	{
		FUnLuaDelegates::OnLuaStateCreated.Remove(RegLuaHandle);
		RegLuaHandle.Reset();
	}

	if (UnRegLuaHandle.IsValid())
	{
		FUnLuaDelegates::OnPostLuaContextCleanup.Remove(UnRegLuaHandle);
		UnRegLuaHandle.Reset();
	}

	if (L)
	{
		lua_sethook(L, nullptr, 0, 0);
		L = nullptr;
	}
}


void UScriptHookClient::RegisterLuaState(lua_State* State)
{
	L = State;

	hook_mode = EHookMode::H_Continue;

	const FString IPStr("127.0.0.1");
	if (FIPv4Address::Parse(IPStr, ServerIp))
	{

		//craete client
		ServerAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		ServerAddr->SetIp(ServerIp.Value);
		ServerAddr->SetPort(8890);

		HostSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("HostSocket"), false);

		if (HostSocket->Connect(*ServerAddr))
		{
			UE_LOG(LogTemp, Log, TEXT("Remote HostSocket Connect Succeed"));

			//open receive thread
			HookReceiveThread = FRunnableThread::Create(new FHookReceiveThread(), TEXT("HookReceiveThread"));

			//stop game and wait server to call
			FSlateApplication::Get().EnterDebuggingMode();

		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Remote HostSocket Connect Failed"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Remote Parse IPStr Error %s"), *IPStr);
	}
}

void UScriptHookClient::UnRegisterLuaState(bool bFullCleanup)
{
	//tell hook server to exit client
	SendClientExit();

	if (L == nullptr)
		return;

	lua_sethook(L, nullptr, 0, 0);

	if (bFullCleanup)
		L = nullptr;

	//destroy TickDelete
	if (TickerHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	}
	//destroy receive thread
	if (HookReceiveThread)
	{
		HookReceiveThread->Kill(true);
		delete HookReceiveThread;
		HookReceiveThread = nullptr;
	}
	//destroy client
	if (HostSocket)
	{
		HostSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(HostSocket);
		HostSocket = nullptr;
	}

}


bool UScriptHookClient::HookReceiveListener()
{
	//UE_LOG(LogTemp, Log, TEXT("Remote HookReceiveListener"));

	if (!HostSocket)
		return true;

	//Binary Array!
	TArray<uint8> ReceivedData;

	uint32 Size;
	while (HostSocket->HasPendingData(Size))
	{
		ReceivedData.Init(0, FMath::Min(Size, 65507u));

		int32 BytesRead = 0;
		HostSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead);

		flatbuffers::FlatBufferBuilder HookDealBuilder;
		HookDealBuilder.PushBytes(ReceivedData.GetData(), BytesRead);
		flatbuffers::Verifier HookDealVerifier(HookDealBuilder.GetCurrentBufferPointer(), HookDealBuilder.GetSize());
		if (!NScriptHook::VerifyFHookDealBuffer(HookDealVerifier))
		{
			UE_LOG(LogTemp, Log, TEXT("Remote HookDealVerifier failed"));
			continue;
		}

		UE_LOG(LogTemp, Log, TEXT("Remote ReceiveData [%d]"), BytesRead);

		auto HookDeal = NScriptHook::GetFHookDeal(HookDealBuilder.GetCurrentBufferPointer());

		switch (HookDeal->DealOrder())
		{
		case EDealOrder::O_Default:
			break;
		case EDealOrder::O_InitData:
			ReceInitData(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_BreakPoints:
			ReceBreakPoints(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_ReqStack:
			ReceReqStack(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_ReqChild:
			ReceReqChild(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_Continue:
			ReceContinue();
			break;
		case EDealOrder::O_StepIn:
			ReceStepIn();
			break;
		case EDealOrder::O_StepOver:
			ReceStepOver();
			break;
		case EDealOrder::O_StepOut:
			ReceStepOut();
			break;
		}

		//delete HookDeal;
	}

	return false;
}


bool UScriptHookClient::HasBreakPoint(FString FilePath, int32 Line)
{
	FString FileName = FPaths::GetCleanFilename(FilePath);

	for (TMap<FString, TSet<int32>>::TIterator It(EnableBreakPoint); It; ++It)
	{
		if (FPaths::GetCleanFilename(It->Key).Equals(FileName))
		{
			if (It->Value.Contains(Line))
			{
				return true;
			}
		}
	}
	return false;
}


FString UScriptHookClient::StringFromBinaryArray(TArray<uint8>& BinaryArray)
{
	BinaryArray.Add(0); // Add 0 termination. Even if the string is already 0-terminated, it doesn't change the results.
	// Create a string from a byte array. The string is expected to be 0 terminated (i.e. a byte set to 0).
	// Use UTF8_TO_TCHAR if needed.
	// If you happen to know the data is UTF-16 (USC2) formatted, you do not need any conversion to begin with.
	// Otherwise you might have to write your own conversion algorithm to convert between multilingual UTF-16 planes.
	return FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(BinaryArray.GetData())));
}

void UScriptHookClient::ReceInitData(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceInitData"));

	ReceBreakPoints(BinaryPointer, BinarySize);

	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		// run game and bind tick
		FSlateApplication::Get().LeaveDebuggingMode();
	}, TStatId(), NULL, ENamedThreads::GameThread);
}

void UScriptHookClient::ReceBreakPoints(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceBreakPoints"));

	flatbuffers::FlatBufferBuilder BreakPointsBuilder;
	BreakPointsBuilder.PushBytes(BinaryPointer, BinarySize);

	flatbuffers::Verifier BreakPointsVerifier(BreakPointsBuilder.GetCurrentBufferPointer(), BreakPointsBuilder.GetSize());
	if (!NScriptHook::VerifyFBreakPointsBuffer(BreakPointsVerifier))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceBreakPoints Verifier failed"));
		return;
	}

	auto BreakPoints = NScriptHook::GetFBreakPoints(BreakPointsBuilder.GetCurrentBufferPointer());

	//lock BreakPoints and L
	FScopeLock* QueueLock = new FScopeLock(&QueueCritical);
	if (BreakPoints->Group()->Length() > 0)
	{
		if (HostScriptPath.IsEmpty())
		{
			FString TempStr;
			FString FullPath = ((*BreakPoints->Group())[0]->FilePath()->c_str());
			FullPath.Split(ScriptMask, &HostScriptPath, &TempStr);
			HostScriptPath = FPaths::Combine(HostScriptPath, ScriptMask);

			//UE_LOG(LogTemp, Log, TEXT("Remote HostScriptPath %s"), *HostScriptPath);
		}

		EnableBreakPoint.Empty();
		for (auto BreakPoint : *BreakPoints->Group())
		{
			TSet<int32>& Lines = EnableBreakPoint.Add(BreakPoint->FilePath()->c_str());
			for (flatbuffers::uoffset_t i = 0; i < BreakPoint->Lines()->Length(); ++i)
			{
				Lines.Add(BreakPoint->Lines()->Get(i));
			}
		}

		//check Lua Hook Exit and bind hook
		if (L && lua_gethook(L) == nullptr)
			lua_sethook(L, debugger_hook_c, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
#if 0
		for (TMap<FString, TSet<int32>>::TIterator It(EnableBreakPoint); It; ++It)
		{
			for (auto Line : It->Value)
			{
				UE_LOG(LogTemp, Log, TEXT("Remote SyncBreakPoints Line[%d] FilePath[%s]"), Line, *It->Key);
	}
}
#endif
	}
	else
	{
		//check Lua Hook Exit and unbind hook
		if (L && lua_gethook(L))
			lua_sethook(L, nullptr, 0, 0);
	}
	//unlock BreakPoints and L
	delete QueueLock;

	//delete BreakPoints;
}


void UScriptHookClient::ReceContinue()
{
	if (L == nullptr)
		return;

	hook_mode = EHookMode::H_Continue;

	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		FSlateApplication::Get().LeaveDebuggingMode();
	}, TStatId(), NULL, ENamedThreads::GameThread);
}

void UScriptHookClient::ReceStepOver()
{
	if (L == nullptr)
		return;

	u_over.Reset();
	hook_mode = EHookMode::H_StepOver;
	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		FSlateApplication::Get().LeaveDebuggingMode();
	}, TStatId(), NULL, ENamedThreads::GameThread);
}

void UScriptHookClient::ReceStepIn()
{
	if (L == nullptr)
		return;

	hook_mode = EHookMode::H_StepIn;
	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		FSlateApplication::Get().LeaveDebuggingMode();
	}, TStatId(), NULL, ENamedThreads::GameThread);
}

void UScriptHookClient::ReceStepOut()
{
	if (L == nullptr)
		return;

	u_out.Reset();
	hook_mode = EHookMode::H_StepOut;
	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		FSlateApplication::Get().LeaveDebuggingMode();
	}, TStatId(), NULL, ENamedThreads::GameThread);
}

void UScriptHookClient::ReceReqStack(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceReqStack"));

	flatbuffers::FlatBufferBuilder ReqStackBuilder;
	ReqStackBuilder.PushBytes(BinaryPointer, BinarySize);

	flatbuffers::Verifier ReqStackVerifier(ReqStackBuilder.GetCurrentBufferPointer(), ReqStackBuilder.GetSize());
	if (!NScriptHook::VerifyFReqStackBuffer(ReqStackVerifier))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceReqStack Verifier failed"));
		return;
	}

	auto ReqStack = NScriptHook::GetFReqStack(ReqStackBuilder.GetCurrentBufferPointer());

	GetStackVars(ReqStack->StackIndex());

	flatbuffers::FlatBufferBuilder StackVarsBuilder;
	std::vector<flatbuffers::Offset<NScriptHook::FVarNode>> VarParentList;
	int i = 0;
	for (auto VarParent : VarList)
	{
		VarParent->NodeMask.Add(i);

		std::vector<flatbuffers::Offset<NScriptHook::FVarNode>> VarChildrenList;
		int j = 0;
		for (TMap<FString, TSharedRef<FScriptDebuggerVarNode>>::TIterator VarChild(VarParent->NodeChildren); VarChild; ++VarChild)
		{
			VarChild->Value->NodeMask.Add(i);
			VarChild->Value->NodeMask.Add(j);

			std::vector<int32> NodeChildMask;
			NodeChildMask.push_back(i);
			NodeChildMask.push_back(j);

			auto VarChildNode = NScriptHook::CreateFVarNode(StackVarsBuilder, StackVarsBuilder.CreateVector(NodeChildMask), StackVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarName.ToString())), StackVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarValue.ToString())), StackVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarType.ToString())));

			VarChildrenList.push_back(VarChildNode);

			j++;
		}
		std::vector<int32> NodeParentMask;
		NodeParentMask.push_back(i);
		auto VarParentNode = NScriptHook::CreateFVarNode(StackVarsBuilder, StackVarsBuilder.CreateVector(NodeParentMask), StackVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarParent->VarName.ToString())), StackVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarParent->VarValue.ToString())), StackVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarParent->VarType.ToString())), StackVarsBuilder.CreateVector(VarChildrenList));

		VarParentList.push_back(VarParentNode);

		i++;
	}

	auto StackVars = NScriptHook::CreateFStackVars(StackVarsBuilder, StackVarsBuilder.CreateVector(VarParentList));
	StackVarsBuilder.Finish(StackVars);

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_StackVars, HookDealBuilder.CreateVector(StackVarsBuilder.GetBufferPointer(), StackVarsBuilder.GetSize()));
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	HostSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendStackVars SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptHookClient::ReceReqChild(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceReqChild"));

	flatbuffers::FlatBufferBuilder ReqChildBuilder;
	ReqChildBuilder.PushBytes(BinaryPointer, BinarySize);

	flatbuffers::Verifier ReqChildVerifier(ReqChildBuilder.GetCurrentBufferPointer(), ReqChildBuilder.GetSize());
	if (!NScriptHook::VerifyFReqChildBuffer(ReqChildVerifier))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceReqChild Verifier failed"));
		return;
	}

	auto ReqChild = NScriptHook::GetFReqChild(ReqChildBuilder.GetCurrentBufferPointer());

	FString NodeMaskStr;
	for (auto NodeIndex : *ReqChild->NodeMask())
	{
		NodeMaskStr += FString::Printf(TEXT(" --> %d"), NodeIndex);
	}
	//UE_LOG(LogTemp, Log, TEXT("Remote NodeMask [%s]"), *NodeMaskStr);

	std::vector<int32> NodeParentMask;
	NodeParentMask.push_back((*ReqChild->NodeMask())[0]);
	//get the node
	FScriptDebuggerVarNode_Ref ParentNode = VarList[(*ReqChild->NodeMask())[0]];
	uint32 i = 1;
	while (i < ReqChild->NodeMask()->size())
	{
		TArray<FScriptDebuggerVarNode_Ref> ChildNodes;
		ParentNode->NodeChildren.GenerateValueArray(ChildNodes);
		if ((*ReqChild->NodeMask())[i] >= ChildNodes.Num())
			return;
		ParentNode = ChildNodes[(*ReqChild->NodeMask())[i]];
		NodeParentMask.push_back((*ReqChild->NodeMask())[i]);

		i++;
	}

	//get node children
	GetVarsChildren(ParentNode.Get());

	//Send ChildVars
	flatbuffers::FlatBufferBuilder ChildVarsBuilder;

	std::vector<flatbuffers::Offset<NScriptHook::FVarNode>> VarChildrenList;
	int j = 0;
	for (TMap<FString, TSharedRef<FScriptDebuggerVarNode>>::TIterator VarChild(ParentNode->NodeChildren); VarChild; ++VarChild)
	{
		VarChild->Value->NodeMask.Reset();
		VarChild->Value->NodeMask.Append(ParentNode->NodeMask);
		VarChild->Value->NodeMask.Add(j);

		std::vector<int32> NodeChildMask;
		NodeChildMask.reserve(ReqChild->NodeMask()->size() + 1);
		NodeChildMask.insert(NodeChildMask.end(), ReqChild->NodeMask()->begin(), ReqChild->NodeMask()->end());
		NodeChildMask.push_back(j);

		auto VarChildNode = NScriptHook::CreateFVarNode(ChildVarsBuilder, ChildVarsBuilder.CreateVector(NodeChildMask), ChildVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarName.ToString())), ChildVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarValue.ToString())), ChildVarsBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarType.ToString())));

		VarChildrenList.push_back(VarChildNode);

		//UE_LOG(LogTemp, Log, TEXT("Remote ParentVar [%s] ChildVars [%s]"), *ParentNode->ToString(), *VarChild->Value->ToString());

		j++;
	}

	auto ChildVars = NScriptHook::CreateFChildVars(ChildVarsBuilder, ChildVarsBuilder.CreateVector(NodeParentMask), ChildVarsBuilder.CreateVector(VarChildrenList));
	ChildVarsBuilder.Finish(ChildVars);

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_ChildVars, HookDealBuilder.CreateVector(ChildVarsBuilder.GetBufferPointer(), ChildVarsBuilder.GetSize()));
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	HostSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendChildVars SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptHookClient::GetStackVars(int32 StackIndex)
{
	VarList.Reset();

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

	VarList.Add(LocalNode);
	VarList.Add(UpValueNode);
	VarList.Add(GlobalNode);

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

									VarList.Add(UEObjectNode);
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

	//iter Varlist and get their children
	for (auto VarNode : VarList)
	{
		GetVarsChildren(*VarNode);
	}
}

void UScriptHookClient::GetVarsChildren(FScriptDebuggerVarNode& InNode)
{
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

bool UScriptHookClient::NameTranslate(int32 KindType, FString& VarName, int32 StackIndex)
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

void UScriptHookClient::ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex)
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

void UScriptHookClient::IteraionTable(FScriptDebuggerVarNode& InNode, int32 NameIndex)
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

void UScriptHookClient::LocalListen(FScriptDebuggerVarNode& InNode)
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

void UScriptHookClient::UpvalueListen(FScriptDebuggerVarNode& InNode)
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

void UScriptHookClient::GlobalListen(FScriptDebuggerVarNode& InNode)
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

bool UScriptHookClient::PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, UProperty* Property, UObject* Object)
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

// UEObjectListen get Object Info by FClassDesc
void UScriptHookClient::UEObjectListen(FScriptDebuggerVarNode& InNode)
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
			for (TFieldIterator<UProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

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
			for (TFieldIterator<UProperty> ProIt(Object->GetClass()); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

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
			for (TFieldIterator<UProperty> ProIt(UEObject->GetClass()); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

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
				UProperty* Property = PropertyDesc->GetProperty();
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
			for (TFieldIterator<UProperty> ProIt(Function); ProIt; ++ProIt)
			{
				UProperty* Property = *ProIt;

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

void UScriptHookClient::SendEnterDebug(FString FilePath, int32 Line)
{
	if (!HostSocket)
		return;

	flatbuffers::FlatBufferBuilder DebugInfoBuilder;
	std::vector<flatbuffers::Offset<NScriptHook::FStackNode>> StackNodeList;

	// get stack info
	int i = 0;
	lua_Debug ar;
	while (lua_getstack(L, i, &ar) != 0 && i < 10)
	{
		if (lua_getinfo(L, "Snl", &ar) != 0)
		{
			FString StackFilePath(UTF8_TO_TCHAR(++ar.source));
			FString StackFuncInfo(UTF8_TO_TCHAR(ar.name));
			auto StackNode = NScriptHook::CreateFStackNode(DebugInfoBuilder, i, ar.currentline, DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*StackFilePath)), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*StackFuncInfo)));
			StackNodeList.push_back(StackNode);
		}
		i++;
	}

	// get StackIndex 0 Var info
	GetStackVars(0);

	std::vector<flatbuffers::Offset<NScriptHook::FVarNode>> VarParentList;
	i = 0;
	for (auto VarParent : VarList)
	{
		VarParent->NodeMask.Add(i);

		std::vector<flatbuffers::Offset<NScriptHook::FVarNode>> VarChildrenList;
		int j = 0;
		for (TMap<FString, TSharedRef<FScriptDebuggerVarNode>>::TIterator VarChild(VarParent->NodeChildren); VarChild; ++VarChild)
		{
			VarChild->Value->NodeMask.Add(i);
			VarChild->Value->NodeMask.Add(j);

			std::vector<int32> NodeChildMask;
			NodeChildMask.push_back(i);
			NodeChildMask.push_back(j);

			auto VarChildNode = NScriptHook::CreateFVarNode(DebugInfoBuilder, DebugInfoBuilder.CreateVector(NodeChildMask), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarName.ToString())), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarValue.ToString())), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*VarChild->Value->VarType.ToString())));

			VarChildrenList.push_back(VarChildNode);

			j++;
		}
		std::vector<int32> NodeParentMask;
		NodeParentMask.push_back(i);
		auto VarParentNode = NScriptHook::CreateFVarNode(DebugInfoBuilder, DebugInfoBuilder.CreateVector(NodeParentMask), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*VarParent->VarName.ToString())), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*VarParent->VarValue.ToString())), DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*VarParent->VarType.ToString())), DebugInfoBuilder.CreateVector(VarChildrenList));

		VarParentList.push_back(VarParentNode);

		i++;
	}

	FString HostFilePath;
	FString LocalFileName;
	FilePath.Split(ScriptMask, &HostFilePath, &LocalFileName);
	HostFilePath = FPaths::Combine(HostScriptPath, LocalFileName);
	//UE_LOG(LogTemp, Log, TEXT("Remote HostFilePath %s"), *HostFilePath);

	auto DebugInfo = NScriptHook::CreateFDebugInfo(DebugInfoBuilder, DebugInfoBuilder.CreateString(TCHAR_TO_UTF8(*HostFilePath)), Line, DebugInfoBuilder.CreateVector(StackNodeList), DebugInfoBuilder.CreateVector(VarParentList));
	DebugInfoBuilder.Finish(DebugInfo);

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_EnterDebug, HookDealBuilder.CreateVector(DebugInfoBuilder.GetBufferPointer(), DebugInfoBuilder.GetSize()));
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	HostSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendEnterDebug SendData [%d]"), HookDealBuilder.GetSize());

	//main thread
	FSlateApplication::Get().EnterDebuggingMode();
}

void UScriptHookClient::SendClientExit()
{
	if (!HostSocket)
		return;

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_ClientExit);
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	HostSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendBreakPoints SendData [%d]"), HookDealBuilder.GetSize());
}


/********Hook Debug Statement********/

//#define UNLUA_DEBUG
#ifdef UNLUA_DEBUG
static unlua_Debug ur;
#else
static unlua_State ur;
#endif // UNLUA_DEBUG

//#define UNFOLD_FUNCTION

static unlua_over u_over;

static unlua_out u_out;

static EHookMode hook_mode = EHookMode::H_Continue;

void UScriptHookClient::hook_call_option()
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

void UScriptHookClient::hook_ret_option()
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

void UScriptHookClient::hook_line_option()
{
	FString HostPath;
	switch (hook_mode)
	{
	case EHookMode::H_Continue:
		if (HasBreakPoint(ur.source, ur.currentline))
		{
			SendEnterDebug(ur.source, ur.currentline);
		}
		break;
	case EHookMode::H_StepIn:
		SendEnterDebug(ur.source, ur.currentline);
		break;
	case EHookMode::H_StepOver:
		if (u_over.BreakPoint())
		{
			SendEnterDebug(ur.source, ur.currentline);
			return;
		}
		if (HasBreakPoint(ur.source, ur.currentline))
		{
			SendEnterDebug(ur.source, ur.currentline);
			return;
		}
		break;
	case EHookMode::H_StepOut:
		if (u_out.BreakPoint())
		{
			SendEnterDebug(ur.source, ur.currentline);
			return;
		}
		if (HasBreakPoint(ur.source, ur.currentline))
		{
			SendEnterDebug(ur.source, ur.currentline);
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
		UE_LOG(LogTemp, Log, TEXT("Remote %s"), *ur.ToString());
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
			UScriptHookClient::Get()->hook_call_option();
			break;
		case LUA_HOOKRET:
			UScriptHookClient::Get()->hook_ret_option();
			break;
		case LUA_HOOKLINE:
			UScriptHookClient::Get()->hook_line_option();
			break;
		}
}
}


#undef UNLUA_DEBUG
#undef UNFOLD_FUNCTION