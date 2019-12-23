#include "UScriptRemoteDebuggerSetting.h"
#include "Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "Networking/Public/Common/UdpSocketBuilder.h"
#include "NoExportTypes.h"
#include "Networking/Public/Common/UdpSocketReceiver.h"
#include "Sockets.h"
#include "Networking/Public/Common/TcpSocketBuilder.h"

void FConnectionListenerAsyncTask::DoWork()
{
	while (!IsStop)
	{
		FPlatformProcess::Sleep(100);
		UScriptRemoteDebuggerSetting::Get()->TCPConnectionListener();
	}
}

void FReceiveListenerAsyncTask::DoWork()
{
	while (!IsStop)
	{
		FPlatformProcess::Sleep(100);
		UScriptRemoteDebuggerSetting::Get()->TCPSocketListener();
	}
}

UScriptRemoteDebuggerSetting* UScriptRemoteDebuggerSetting::Get()
{
	static UScriptRemoteDebuggerSetting* Singleton = nullptr;
	if (Singleton == nullptr)
	{
		Singleton = NewObject<UScriptRemoteDebuggerSetting>();
		Singleton->AddToRoot();

		//start listener tcp
		if (Singleton->StartTCPReceiver("HookListener", "127.0.0.1", 8890))
		{
			UE_LOG(LogTemp, Log, TEXT("Remote StartTCPReceiver HookListener Succeed"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Remote StartTCPReceiver HookListener Failed"));
		}
	}
	return Singleton;
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
	//new FAutoDeleteAsyncTask<ExampleAutoDeleteAsyncTask>(5)->StartBackgroundTask()

	return true;
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

void UScriptRemoteDebuggerSetting::TCPConnectionListener()
{
	//~~~~~~~~~~~~~
	if (!ListenerSocket) return;
	//~~~~~~~~~~~~~

	//Remote address
	TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool Pending;

	// handle incoming connections
	if (ListenerSocket->HasPendingConnection(Pending) && Pending)
	{
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//Already have a Connection? destroy previous
		if (ConnectionSocket)
		{
			ConnectionSocket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket);
		}
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		//New Connection receive!
		ConnectionSocket = ListenerSocket->Accept(*RemoteAddress, TEXT("RamaTCP Received Socket Connection"));

		if (ConnectionSocket != NULL)
		{
			//Global cache of current Remote Address
			RemoteAddressForConnection = FIPv4Endpoint(RemoteAddress);

			//UE_LOG "Accepted Connection! WOOOHOOOO!!!";

			//can thread this too
			//GetWorldTimerManager().SetTimer(this, &AYourClass::TCPSocketListener, 0.01, true);
		}
	}
}

void UScriptRemoteDebuggerSetting::TCPSocketListener()
{
	//~~~~~~~~~~~~~
	if (!ConnectionSocket) return;
	//~~~~~~~~~~~~~


	//Binary Array!
	TArray<uint8> ReceivedData;

	uint32 Size;
	while (ConnectionSocket->HasPendingData(Size))
	{
		//ReceivedData.Init(FMath::Min(Size, 65507u));

		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);

		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Data Read! %d"), ReceivedData.Num()));
	}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (ReceivedData.Num() <= 0)
	{
		//No Data Received
		return;
	}


	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//						Rama's String From Binary Array
	const FString ReceivedUE4String = StringFromBinaryArray(ReceivedData);
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



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

