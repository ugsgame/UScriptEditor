#pragma once

#include "CoreMinimal.h"
#include "UScriptDebuggerSetting.h"
#include "Networking/Public/Interfaces/IPv4/IPv4Endpoint.h"
#include "Runnable.h"
#include "ScriptHookType.h"
#include "UScriptRemoteDebuggerSetting.generated.h"

class FSocket;

class FConnectionListenerWorker : public FRunnable
{

public:

	FThreadSafeCounter StopTaskCounter;

public:

	FConnectionListenerWorker() : StopTaskCounter(0) {}

	virtual uint32 Run() override;

	virtual void Stop() override;

};

class FReceiveListenerWorker : public FRunnable
{
public:

	FThreadSafeCounter StopTaskCounter;

public:

	FReceiveListenerWorker() : StopTaskCounter(0) {}

	virtual uint32 Run() override;

	virtual void Stop() override;

};



UCLASS()
class  UScriptRemoteDebuggerSetting : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
		static UScriptRemoteDebuggerSetting* Get();

	void SetTabIsOpen(bool IsOpen);

	void CreateHookServer();

	void DestroyHookServer();

	//Timer functions, could be threads
	bool TCPConnectionListener(); 	

	bool TCPReceiveListener();

	void SendInitData();

	void SendBreakPoints();

	void SendContinue();

	void SendStepOver();

	void SendStepIn();

	void SendStepOut();

	void SendReqStack(int32 StackIndex);

	void GetVarsChildren(FScriptDebuggerVarNode& InNode);

public:

	TArray<TTuple<int32, int32, FString, FString>> StackInfos;

	TArray<FScriptDebuggerVarNode_Ref> LocalVarInfos;

	FString LuaFilePath;

	int32 LuaCodeLine;

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

	void ReceClientExit();

	void ReceEnterDebug(const uint8* BinaryPointer, uint32_t BinarySize);

	void ReceStackVars(const uint8* BinaryPointer, uint32_t BinarySize);

	void ReceChildVars(const uint8* BinaryPointer, uint32_t BinarySize);

protected:

	FSocket* ListenerSocket;
	FSocket* ClientSocket;
	FIPv4Endpoint RemoteAddressForConnection;

	bool IsTapOpen;

	FRunnableThread* ConnectionListenerThread;

	FRunnableThread* ReceiveListenerThread;

	FCriticalSection QueueCritical;

	TArray<FString> ReqChildHistory;

	TArray<FString> ReqChildQueue;

	TArray<uint8> ReceivedData;

	uint32 TotalSize = 0;
};
