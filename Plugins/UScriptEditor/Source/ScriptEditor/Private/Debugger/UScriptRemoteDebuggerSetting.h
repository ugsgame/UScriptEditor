#pragma once

#include "CoreMinimal.h"
#include "UScriptDebuggerSetting.h"
#include "Networking/Public/Interfaces/IPv4/IPv4Endpoint.h"
#include "UScriptRemoteDebuggerSetting.generated.h"

class FSocket;

class FConnectionListenerAsyncTask : public FNonAbandonableTask
{
public:

	friend class FAutoDeleteAsyncTask<FConnectionListenerAsyncTask>;

	bool IsStop;

	FConnectionListenerAsyncTask() : IsStop(false) {};

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FConnectionListenerAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};

class FReceiveListenerAsyncTask : public FNonAbandonableTask
{
public:

	friend class FAutoDeleteAsyncTask<FReceiveListenerAsyncTask>;

	bool IsStop;

	FReceiveListenerAsyncTask() : IsStop(false) {};

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FReceiveListenerAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};


UCLASS()
class  UScriptRemoteDebuggerSetting : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
		static UScriptRemoteDebuggerSetting* Get();

	//Timer functions, could be threads
	void TCPConnectionListener(); 	//can thread this eventually
	void TCPSocketListener();		//can thread this eventually

protected:

	bool StartTCPReceiver(
		const FString& YourChosenSocketName,
		const FString& TheIP,
		const int32 ThePort
	);

	FSocket* CreateTCPConnectionListener(
		const FString& YourChosenSocketName,
		const FString& TheIP,
		const int32 ThePort,
		const int32 ReceiveBufferSize = 2 * 1024 * 1024
	);


	//Format String IP4 to number array
	bool FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4]);

	//Rama's StringFromBinaryArray
	FString StringFromBinaryArray(TArray<uint8>& BinaryArray);

protected:

	FSocket* ListenerSocket;
	FSocket* ConnectionSocket;
	FIPv4Endpoint RemoteAddressForConnection;

	FConnectionListenerAsyncTask* ConnectionListenerAsyncTask;
	FReceiveListenerAsyncTask* ReceiveListenerAsyncTask;
};
