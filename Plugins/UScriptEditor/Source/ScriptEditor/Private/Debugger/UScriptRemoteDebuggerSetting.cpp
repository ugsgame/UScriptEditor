#include "UScriptRemoteDebuggerSetting.h"
#include "Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "Networking/Public/Common/UdpSocketBuilder.h"
#include "NoExportTypes.h"
#include "Networking/Public/Common/UdpSocketReceiver.h"
#include "Sockets.h"
#include "Networking/Public/Common/TcpSocketBuilder.h"
#include "ScriptHookDeal.h"
#include "ScriptBreakPoints.h"
#include "ScriptEditor.h"
#include "ScriptDebugInfo.h"
#include "ScriptReqStack.h"
#include "ScriptStackVars.h"
#include "Async.h"
#include "SlateApplication.h"
#include "ScriptReqChild.h"
#include "ScriptChildVars.h"

#if 0
class FRefreshVarsTask
{
public:
	FRefreshVarsTask() {}
	static const TCHAR* GetTaskName()
	{
		return TEXT("FRefreshVarsTask");
	}
	FORCEINLINE static TStatId GetStatId()
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FRefreshVarsTask, STATGROUP_TaskGraphTasks);
	}
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::GameThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		SScriptDebugger::Get()->SetStackData(UScriptRemoteDebuggerSetting::Get()->StackInfos);
		SScriptDebugger::Get()->EnterDebug(UScriptRemoteDebuggerSetting::Get()->LuaFilePath, UScriptRemoteDebuggerSetting::Get()->LuaCodeLine);
		SScriptDebugger::Get()->RemoteRefreshVars(UScriptRemoteDebuggerSetting::Get()->VarInfos);
	}
};
#endif

uint32 FConnectionListenerWorker::Run()
{
	while (StopTaskCounter.GetValue() == 0)
	{
		FPlatformProcess::Sleep(0.3f);
		if (UScriptRemoteDebuggerSetting::Get()->TCPConnectionListener())
			StopTaskCounter.Increment();
	}
	return 0;
}

void FConnectionListenerWorker::Stop()
{
	StopTaskCounter.Increment();

	UE_LOG(LogTemp, Log, TEXT("Remote ConnectionListenerThread Stop"));
}

uint32 FReceiveListenerWorker::Run()
{
	while (StopTaskCounter.GetValue() == 0)
	{
		FPlatformProcess::Sleep(0.01f);
		if (UScriptRemoteDebuggerSetting::Get()->TCPReceiveListener())
			StopTaskCounter.Increment();
	}
	return 0;
}

void FReceiveListenerWorker::Stop()
{
	StopTaskCounter.Increment();

	UE_LOG(LogTemp, Log, TEXT("Remote ReceiveListenerThread Stop"));
}



UScriptRemoteDebuggerSetting* UScriptRemoteDebuggerSetting::Get()
{
	static UScriptRemoteDebuggerSetting* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptRemoteDebuggerSetting>();
		Singleton->AddToRoot();

		//bind slateapplication tick
		//FSlateApplication::Get().OnPreTick().AddUObject(Singleton, &UScriptRemoteDebuggerSetting::SlateTick);
	}
	return Singleton;
}

void UScriptRemoteDebuggerSetting::SetTabIsOpen(bool IsOpen)
{
	IsTapOpen = IsOpen;

	if (!IsTapOpen)
		DestroyHookServer();
}

void UScriptRemoteDebuggerSetting::CreateHookServer()
{
	//start listener tcp
	if (StartTCPReceiver("HookListener", "127.0.0.1", 8890))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote StartTCPReceiver HookListener Succeed"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Remote StartTCPReceiver HookListener Failed"));
	}
}

void UScriptRemoteDebuggerSetting::DestroyHookServer()
{

	if (ConnectionListenerThread)
	{
		ConnectionListenerThread->Kill(true);
		delete ConnectionListenerThread;
		ConnectionListenerThread = NULL;
	}

	if (ReceiveListenerThread)
	{
		ReceiveListenerThread->Kill(true);
		delete ReceiveListenerThread;
		ReceiveListenerThread = NULL;
	}

	if (ClientSocket)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = NULL;
	}

	if (ListenerSocket)
	{
		ListenerSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
		ListenerSocket = NULL;
	}

	ReqChildHistory.Reset();
	LocalVarInfos.Reset();

	UE_LOG(LogTemp, Log, TEXT("Remote DestroyHookServer"));

}

FSocket* UScriptRemoteDebuggerSetting::CreateTCPConnectionListener(const FString& YourChosenSocketName, const FString& TheIP, const int32 ThePort, const int32 ReceiveBufferSize /*= 2 * 1024 * 1024 */)
{
	uint8 IP4Nums[4];
	if (!FormatIP4ToNumber(TheIP, IP4Nums))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote Invalid IP! Expecting 4 parts separated by ~> %s %d"), *TheIP, ThePort);
		return false;
	}

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//Create Socket
	FIPv4Endpoint Endpoint(FIPv4Address(IP4Nums[0], IP4Nums[1], IP4Nums[2], IP4Nums[3]), ThePort);
	FSocket* ListenSocket = FTcpSocketBuilder(*YourChosenSocketName)
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.Listening(8);

	//Set Buffer Size
	int32 NewSize = 0;
	ListenSocket->SetReceiveBufferSize(ReceiveBufferSize, NewSize);

	//Done!
	return ListenSocket;
}

bool UScriptRemoteDebuggerSetting::StartTCPReceiver(const FString& YourChosenSocketName, const FString& TheIP, const int32 ThePort)
{
	//Rama's CreateTCPConnectionListener
	ListenerSocket = CreateTCPConnectionListener(YourChosenSocketName, TheIP, ThePort);

	//Not created?
	if (!ListenerSocket)
	{
		UE_LOG(LogTemp, Log, TEXT("Remote StartTCPReceiver>> Listen socket could not be created! ~> %s %d"), *TheIP, ThePort);
		return false;
	}

	//Thread to check connent
	ConnectionListenerThread = FRunnableThread::Create(new FConnectionListenerWorker(), TEXT("ConnectionListenerThread"));

	return true;
}

bool UScriptRemoteDebuggerSetting::TCPConnectionListener()
{

	//~~~~~~~~~~~~~
	if (!ListenerSocket)
		return true;
	//~~~~~~~~~~~~~

	//Remote address
	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	// handle incoming connections
	if (ListenerSocket->HasPendingConnection(Pending) && Pending)
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//Already have a Connection? destroy previous
		if (ClientSocket)
		{
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		//New Connection receive!
		ClientSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("RamaTCP Received Socket Connection"));

		if (ClientSocket != NULL)
		{
			//Global cache of current Remote Address
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);

			//send the BreakPoint
			SendInitData();

			ReceiveListenerThread = FRunnableThread::Create(new FReceiveListenerWorker(), TEXT("ReceiveListenerThread"));

			return true;
		}
	}

	return false;
}

bool UScriptRemoteDebuggerSetting::TCPReceiveListener()
{

	if (!ClientSocket)
		return true;

	//Binary Array!
	TArray<uint8> ReceivedData;

	uint32 TotalSize = 0;
	uint32 PendingSize;
	while (ClientSocket->HasPendingData(PendingSize))
	{
		TArray<uint8> TempData;
		TempData.Init(0, PendingSize);

		int32 BytesRead = 0;
		ClientSocket->Recv(TempData.GetData(), TempData.Num(), BytesRead);

		ReceivedData.Append(TempData);
		TotalSize += PendingSize;

		continue;
	}

	if (ReceivedData.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceiveData [%d]"), TotalSize);

		flatbuffers::FlatBufferBuilder HookDealBuilder;
		HookDealBuilder.PushBytes(ReceivedData.GetData(), TotalSize);
		flatbuffers::Verifier HookDealVerifier(HookDealBuilder.GetCurrentBufferPointer(), HookDealBuilder.GetSize());
		if (!NScriptHook::VerifyFHookDealBuffer(HookDealVerifier))
		{
			UE_LOG(LogTemp, Log, TEXT("Remote HookDealVerifier failed"));
			return false;
		}

		auto HookDeal = NScriptHook::GetFHookDeal(HookDealBuilder.GetCurrentBufferPointer());

		switch (HookDeal->DealOrder())
		{
		case EDealOrder::O_Default:
			break;
		case EDealOrder::O_ClientExit:
			ReceClientExit();
			break;
		case EDealOrder::O_EnterDebug:
			ReceEnterDebug(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_StackVars:
			ReceStackVars(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_ChildVars:
			ReceChildVars(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		}
	}

#if 0
	uint32 Size;
	while (ClientSocket->HasPendingData(Size))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceiveData [%d]"), Size);

		//ReceivedData.Init(0, FMath::Min(Size, 65507u));
		ReceivedData.Init(0, Size);

		int32 BytesRead = 0;
		ClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead);

		flatbuffers::FlatBufferBuilder HookDealBuilder;
		HookDealBuilder.PushBytes(ReceivedData.GetData(), BytesRead);
		flatbuffers::Verifier HookDealVerifier(HookDealBuilder.GetCurrentBufferPointer(), HookDealBuilder.GetSize());
		if (!NScriptHook::VerifyFHookDealBuffer(HookDealVerifier))
		{
			UE_LOG(LogTemp, Log, TEXT("Remote HookDealVerifier failed"));
			continue;
		}

		auto HookDeal = NScriptHook::GetFHookDeal(HookDealBuilder.GetCurrentBufferPointer());

		switch (HookDeal->DealOrder())
		{
		case EDealOrder::O_Default:
			break;
		case EDealOrder::O_ClientExit:
			ReceClientExit();
			break;
		case EDealOrder::O_EnterDebug:
			ReceEnterDebug(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_StackVars:
			ReceStackVars(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		case EDealOrder::O_ChildVars:
			ReceChildVars(HookDeal->DealData()->Data(), HookDeal->DealData()->size());
			break;
		}
	}
#endif

	return false;

}

void UScriptRemoteDebuggerSetting::SendInitData()
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder BreakPointsBuilder;
	std::vector<flatbuffers::Offset<NScriptHook::FBreakPoint>> BreakPointList;
	for (TMap<FString, TSet<int32>>::TIterator It(FScriptEditor::Get()->EnableBreakPoint); It; ++It)
	{
		std::vector<int32> Lines;
		for (auto Line : It.Value())
		{
			Lines.push_back(Line);
		}
		auto BreakPoint = NScriptHook::CreateFBreakPoint(BreakPointsBuilder, BreakPointsBuilder.CreateString(TCHAR_TO_UTF8(*It.Key())), BreakPointsBuilder.CreateVector(Lines));
		BreakPointList.push_back(BreakPoint);
	}
	auto BreakPoints = NScriptHook::CreateFBreakPoints(BreakPointsBuilder, BreakPointsBuilder.CreateVector(BreakPointList));
	BreakPointsBuilder.Finish(BreakPoints);

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_InitData, HookDealBuilder.CreateVector(BreakPointsBuilder.GetBufferPointer(), BreakPointsBuilder.GetSize()));
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendInitData SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptRemoteDebuggerSetting::SendBreakPoints()
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder BreakPointsBuilder;
	std::vector<flatbuffers::Offset<NScriptHook::FBreakPoint>> BreakPointList;
	for (TMap<FString, TSet<int32>>::TIterator It(FScriptEditor::Get()->EnableBreakPoint); It; ++It)
	{
		std::vector<int32> Lines;
		for (auto Line : It.Value())
		{
			Lines.push_back(Line);
		}
		auto BreakPoint = NScriptHook::CreateFBreakPoint(BreakPointsBuilder, BreakPointsBuilder.CreateString(TCHAR_TO_UTF8(*It.Key())), BreakPointsBuilder.CreateVector(Lines));
		BreakPointList.push_back(BreakPoint);
	}
	auto BreakPoints = NScriptHook::CreateFBreakPoints(BreakPointsBuilder, BreakPointsBuilder.CreateVector(BreakPointList));
	BreakPointsBuilder.Finish(BreakPoints);

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_BreakPoints, HookDealBuilder.CreateVector(BreakPointsBuilder.GetBufferPointer(), BreakPointsBuilder.GetSize()));
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendBreakPoints SendData [%d]"), HookDealBuilder.GetSize());
}


void UScriptRemoteDebuggerSetting::SendContinue()
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_Continue);
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendContinue SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptRemoteDebuggerSetting::SendStepOver()
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_StepOver);
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendStepOver SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptRemoteDebuggerSetting::SendStepIn()
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_StepIn);
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendStepIn SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptRemoteDebuggerSetting::SendStepOut()
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_StepOut);
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendStepOut SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptRemoteDebuggerSetting::SendReqStack(int32 StackIndex)
{
	if (!ClientSocket)
		return;

	flatbuffers::FlatBufferBuilder ReqStackBuilder;
	auto ReqStack = NScriptHook::CreateFReqStack(ReqStackBuilder, StackIndex);
	ReqStackBuilder.Finish(ReqStack);

	flatbuffers::FlatBufferBuilder HookDealBuilder;
	auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_ReqStack, HookDealBuilder.CreateVector(ReqStackBuilder.GetBufferPointer(), ReqStackBuilder.GetSize()));
	HookDealBuilder.Finish(HookDeal);

	int32 ByteSent = 0;
	ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

	UE_LOG(LogTemp, Log, TEXT("Remote SendReqChild SendData [%d]"), HookDealBuilder.GetSize());
}

void UScriptRemoteDebuggerSetting::GetVarsChildren(FScriptDebuggerVarNode& InNode)
{
	if (!ClientSocket)
		return;

	// do not req root node
	if (InNode.NodeMask.Num() == 1)
		return;

	FString NodeMask = InNode.ToMask();
	if (ReqChildHistory.Contains(NodeMask))
		return;

	//not allow to step
	SScriptDebugger::Get()->IsEnterDebugMode = false;

	//if ReqChildQuene is empty, can send ReqChild in here
	if (ReqChildQueue.Num() == 0)
	{
		flatbuffers::FlatBufferBuilder ReqChildBuilder;
		std::vector<int32> NodeMaskVector;
		for (auto MaskIndex : InNode.NodeMask)
		{
			NodeMaskVector.push_back(MaskIndex);
		}
		auto ReqChild = NScriptHook::CreateFReqChild(ReqChildBuilder, ReqChildBuilder.CreateVector(NodeMaskVector));
		ReqChildBuilder.Finish(ReqChild);

		flatbuffers::FlatBufferBuilder HookDealBuilder;
		auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_ReqChild, HookDealBuilder.CreateVector(ReqChildBuilder.GetBufferPointer(), ReqChildBuilder.GetSize()));
		HookDealBuilder.Finish(HookDeal);

		int32 ByteSent = 0;
		ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

		UE_LOG(LogTemp, Log, TEXT("Remote SendReqChild SendData [%d] ParentNode [%s]"), HookDealBuilder.GetSize(), *InNode.ToString());
	}

	//lock VarInfos
	FScopeLock* QueueLock = new FScopeLock(&QueueCritical);

	ReqChildHistory.Add(NodeMask);
	ReqChildQueue.Add(NodeMask);

	delete QueueLock;
}

bool UScriptRemoteDebuggerSetting::FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4])
{
	//IP Formatting
	TheIP.Replace(TEXT(" "), TEXT(""));

	//String Parts
	TArray<FString> Parts;
	TheIP.ParseIntoArray(Parts, TEXT("."), true);
	if (Parts.Num() != 4)
		return false;

	//String to Number Parts
	for (int32 i = 0; i < 4; ++i)
	{
		Out[i] = FCString::Atoi(*Parts[i]);
	}

	return true;
}

FString UScriptRemoteDebuggerSetting::StringFromBinaryArray(TArray<uint8>& BinaryArray)
{
	BinaryArray.Add(0); // Add 0 termination. Even if the string is already 0-terminated, it doesn't change the results.
	// Create a string from a byte array. The string is expected to be 0 terminated (i.e. a byte set to 0).
	// Use UTF8_TO_TCHAR if needed.
	// If you happen to know the data is UTF-16 (USC2) formatted, you do not need any conversion to begin with.
	// Otherwise you might have to write your own conversion algorithm to convert between multilingual UTF-16 planes.
	return FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(BinaryArray.GetData())));
}

void UScriptRemoteDebuggerSetting::ReceClientExit()
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceClientExit"));

	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		SScriptDebugger::Get()->CleanDebugInfo();

		//close Receive thread
		if (ReceiveListenerThread)
		{
			ReceiveListenerThread->Kill(true);
			delete ReceiveListenerThread;
			ReceiveListenerThread = NULL;
		}
		//close ClientSocket
		if (ClientSocket)
		{
			ClientSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
			ClientSocket = NULL;
		}

		//run ConnectionListenerThread
		ConnectionListenerThread = FRunnableThread::Create(new FConnectionListenerWorker(), TEXT("ConnectionListenerThread"));

		ReqChildHistory.Reset();
		LocalVarInfos.Reset();

	}, TStatId(), NULL, ENamedThreads::GameThread);
}

void UScriptRemoteDebuggerSetting::ReceEnterDebug(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceEnterDebug"));

	//lock StackInfos and VarInfos
	FScopeLock* QueueLock = new FScopeLock(&QueueCritical);

	flatbuffers::FlatBufferBuilder DebugInfoBuilder;
	DebugInfoBuilder.PushBytes(BinaryPointer, BinarySize);

	flatbuffers::Verifier DebugInfoVerifier(DebugInfoBuilder.GetCurrentBufferPointer(), DebugInfoBuilder.GetSize());
	if (!NScriptHook::VerifyFDebugInfoBuffer(DebugInfoVerifier))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceEnterDebug Verifier failed"));
		return;
	}

	auto DebugInfo = NScriptHook::GetFDebugInfo(DebugInfoBuilder.GetCurrentBufferPointer());

	LuaFilePath = UTF8_TO_TCHAR(DebugInfo->FilePath()->c_str());
	LuaCodeLine = DebugInfo->Line();

	StackInfos.Reset();
	for (auto StackNode : *DebugInfo->StackList())
	{
		TTuple<int32, int32, FString, FString> StackItem(StackNode->StackIndex(), StackNode->Line(), UTF8_TO_TCHAR(StackNode->FilePath()->c_str()), UTF8_TO_TCHAR(StackNode->FuncInfo()->c_str()));
		StackInfos.Add(StackItem);
	}

	ReqChildHistory.Reset();
	ReqChildQueue.Reset();
	LocalVarInfos.Reset();
	int i = 0;
	for (auto VarParent : *DebugInfo->VarList())
	{
		FScriptDebuggerVarNode_Ref VarParentNode = MakeShareable(new FScriptDebuggerVarNode);
		VarParentNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarParent->VarName()->c_str()));
		VarParentNode->VarValue = FText::FromString(UTF8_TO_TCHAR(VarParent->VarValue()->c_str()));
		VarParentNode->VarType = FText::FromString(UTF8_TO_TCHAR(VarParent->VarType()->c_str()));
		VarParentNode->NodeMask.Add(i);

		int j = 0;
		for (auto VarChild : *VarParent->NodeChildren())
		{
			FScriptDebuggerVarNode_Ref VarChildNode = MakeShareable(new FScriptDebuggerVarNode);
			VarChildNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarChild->VarName()->c_str()));
			VarChildNode->VarValue = FText::FromString(UTF8_TO_TCHAR(VarChild->VarValue()->c_str()));
			VarChildNode->VarType = FText::FromString(UTF8_TO_TCHAR(VarChild->VarType()->c_str()));
			VarChildNode->NodeMask.Add(i);
			VarChildNode->NodeMask.Add(j);

			VarParentNode->NodeChildren.Add(UTF8_TO_TCHAR(VarChild->VarName()->c_str()), VarChildNode);

			j++;
		}

		LocalVarInfos.Add(VarParentNode);
		//VarInfos.Add(VarParentNode);
		i++;
	}
	//UE_LOG(LogTemp, Log, TEXT("Remote VarInfo %d"), VarParentList.Num());

	//for (auto TVarParent : VarParentList)
	//{
	//	UE_LOG(LogTemp, Log, TEXT("Remote Parent %s"), *TVarParent->ToString());

	//	for (TMap<FString, TSharedRef<FScriptDebuggerVarNode>>::TIterator TVarChild(TVarParent->NodeChildren); TVarChild; ++TVarChild)
	//	{
	//		UE_LOG(LogTemp, Log, TEXT("Remote Children %s"), *TVarChild->Value->ToString());
	//	}
	//}

	//unlock StackInfos and VarInfos
	delete QueueLock;


#if 0

	TGraphTask<FRefreshVarsTask>::CreateTask(NULL, ENamedThreads::GameThread).ConstructAndDispatchWhenReady();

	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		SScriptDebugger::Get()->SetStackData(StackInfos);
		SScriptDebugger::Get()->EnterDebug(LuaFilePath, LuaCodeLine);
		SScriptDebugger::Get()->RemoteRefreshVars(VarInfos);
	}
	);

#else

	UE_LOG(LogTemp, Log, TEXT("Remote EnterDebug [%d]  [%s]"), LuaCodeLine, *LuaFilePath);

	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		SScriptDebugger::Get()->SetStackData(StackInfos);
		SScriptDebugger::Get()->EnterDebug(LuaFilePath, LuaCodeLine);
		SScriptDebugger::Get()->RemoteRefreshVars(LocalVarInfos);

	}, TStatId(), NULL, ENamedThreads::GameThread);

#endif
}

void UScriptRemoteDebuggerSetting::ReceStackVars(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceStackVars"));

	//lock VarInfos
	FScopeLock* QueueLock = new FScopeLock(&QueueCritical);

	flatbuffers::FlatBufferBuilder StackVarsBuilder;
	StackVarsBuilder.PushBytes(BinaryPointer, BinarySize);

	flatbuffers::Verifier StackVarsVerifier(StackVarsBuilder.GetCurrentBufferPointer(), StackVarsBuilder.GetSize());
	if (!NScriptHook::VerifyFStackVarsBuffer(StackVarsVerifier))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceStackVars Verifier failed"));
		return;
	}

	auto StackVars = NScriptHook::GetFStackVars(StackVarsBuilder.GetCurrentBufferPointer());

	ReqChildHistory.Reset();
	ReqChildQueue.Reset();
	LocalVarInfos.Reset();
	int i = 0;
	for (auto VarParent : *StackVars->VarList())
	{
		FScriptDebuggerVarNode_Ref VarParentNode = MakeShareable(new FScriptDebuggerVarNode);
		VarParentNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarParent->VarName()->c_str()));
		VarParentNode->VarValue = FText::FromString(UTF8_TO_TCHAR(VarParent->VarValue()->c_str()));
		VarParentNode->VarType = FText::FromString(UTF8_TO_TCHAR(VarParent->VarType()->c_str()));
		VarParentNode->NodeMask.Add(i);

		int j = 0;
		for (auto VarChild : *VarParent->NodeChildren())
		{
			FScriptDebuggerVarNode_Ref VarChildNode = MakeShareable(new FScriptDebuggerVarNode);
			VarChildNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarChild->VarName()->c_str()));
			VarChildNode->VarValue = FText::FromString(UTF8_TO_TCHAR(VarChild->VarValue()->c_str()));
			VarChildNode->VarType = FText::FromString(UTF8_TO_TCHAR(VarChild->VarType()->c_str()));
			VarChildNode->NodeMask.Add(i);
			VarChildNode->NodeMask.Add(j);

			VarParentNode->NodeChildren.Add(UTF8_TO_TCHAR(VarChild->VarName()->c_str()), VarChildNode);

			j++;
		}

		LocalVarInfos.Add(VarParentNode);
		i++;
	}

	//unlock VarInfos
	delete QueueLock;

	FGraphEventRef GameTask = FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
	{
		SScriptDebugger::Get()->RemoteRefreshVars(LocalVarInfos);

	}, TStatId(), NULL, ENamedThreads::GameThread);

}

void UScriptRemoteDebuggerSetting::ReceChildVars(const uint8* BinaryPointer, uint32_t BinarySize)
{
	UE_LOG(LogTemp, Log, TEXT("Remote ReceChildVars"));

	//lock VarInfos
	FScopeLock* QueueLock = new FScopeLock(&QueueCritical);

	flatbuffers::FlatBufferBuilder ChildVarsBuilder;
	ChildVarsBuilder.PushBytes(BinaryPointer, BinarySize);

	flatbuffers::Verifier ChildVarsVerifier(ChildVarsBuilder.GetCurrentBufferPointer(), ChildVarsBuilder.GetSize());
	if (!NScriptHook::VerifyFChildVarsBuffer(ChildVarsVerifier))
	{
		UE_LOG(LogTemp, Log, TEXT("Remote ReceChildVars Verifier failed"));
		return;
	}

	auto ChildVars = NScriptHook::GetFChildVars(ChildVarsBuilder.GetCurrentBufferPointer());

	FString NodeMaskStr;
	for (auto NodeIndex : *ChildVars->NodeMask())
	{
		NodeMaskStr += FString::Printf(TEXT(" --> %d"), NodeIndex);
	}
	//UE_LOG(LogTemp, Log, TEXT("Remote NodeMask [%s]"), *NodeMaskStr);

	//get the node
	FScriptDebuggerVarNode_Ref ParentNode = LocalVarInfos[(*ChildVars->NodeMask())[0]];
	uint32 i = 1;
	while (i < ChildVars->NodeMask()->size())
	{
		TArray<FScriptDebuggerVarNode_Ref> ChildNodes;
		ParentNode->NodeChildren.GenerateValueArray(ChildNodes);
		if ((*ChildVars->NodeMask())[i] >= ChildNodes.Num())
			return;
		ParentNode = ChildNodes[(*ChildVars->NodeMask())[i]];

		i++;
	}

	//UE_LOG(LogTemp, Log, TEXT("Remote ParentVars [%s]"), *ParentNode->ToString());

	int j = 0;
	for (auto VarChild : *ChildVars->VarList())
	{
		FScriptDebuggerVarNode_Ref VarChildNode = MakeShareable(new FScriptDebuggerVarNode);
		VarChildNode->VarName = FText::FromString(UTF8_TO_TCHAR(VarChild->VarName()->c_str()));
		VarChildNode->VarValue = FText::FromString(UTF8_TO_TCHAR(VarChild->VarValue()->c_str()));
		VarChildNode->VarType = FText::FromString(UTF8_TO_TCHAR(VarChild->VarType()->c_str()));
		VarChildNode->NodeMask.Append(ParentNode->NodeMask);
		VarChildNode->NodeMask.Add(j);

		ParentNode->NodeChildren.Add(UTF8_TO_TCHAR(VarChild->VarName()->c_str()), VarChildNode);

		//UE_LOG(LogTemp, Log, TEXT("Remote ChildVars [%s]"), *VarChildNode->ToString());

		j++;
	}

	//unlock VarInfos
	delete QueueLock;

	if (ReqChildQueue.Num() <= 1)
	{
		ReqChildQueue.Reset();
		//free step
		SScriptDebugger::Get()->IsEnterDebugMode = true;
		return;
	}

	if (!ClientSocket)
		return;

	FString ReceNodeMask = ReqChildQueue[0];
	ReqChildQueue.RemoveAt(0);

	if (ParentNode->ToMask().Equals(ReceNodeMask))
	{
		FString ChildNodeMask = ReqChildQueue[0];

		TArray<FString> NodeMaskArray;
		ChildNodeMask.ParseIntoArray(NodeMaskArray, TEXT(","));

		flatbuffers::FlatBufferBuilder ReqChildBuilder;
		std::vector<int32> NodeMaskVector;
		for (auto MaskIndexStr : NodeMaskArray)
		{
			NodeMaskVector.push_back(FCString::Atoi(*MaskIndexStr));
		}
		auto ReqChild = NScriptHook::CreateFReqChild(ReqChildBuilder, ReqChildBuilder.CreateVector(NodeMaskVector));
		ReqChildBuilder.Finish(ReqChild);

		flatbuffers::FlatBufferBuilder HookDealBuilder;
		auto HookDeal = NScriptHook::CreateFHookDeal(HookDealBuilder, EDealOrder::O_ReqChild, HookDealBuilder.CreateVector(ReqChildBuilder.GetBufferPointer(), ReqChildBuilder.GetSize()));
		HookDealBuilder.Finish(HookDeal);

		int32 ByteSent = 0;
		ClientSocket->Send(HookDealBuilder.GetBufferPointer(), HookDealBuilder.GetSize(), ByteSent);

		UE_LOG(LogTemp, Log, TEXT("Remote SendReqChild SendData [%d] ChildNodeMask [%s]"), HookDealBuilder.GetSize(), *ChildNodeMask);
	}
}

