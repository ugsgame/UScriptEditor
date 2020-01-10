// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IPAddress.h"
#include "ScriptHookType.h"
#include "Networking/Public/Interfaces/IPv4/IPv4Address.h"
#include "ScriptHookClient.generated.h"

class FSocket;

/**
 *
 */
UCLASS(Config = Game)
class SCRIPTHOOK_API UScriptHookClient : public UObject
{
	GENERATED_BODY()

public:

	static UScriptHookClient* Get();

	void BindDebugState();

	void UnBindDebugState();

	bool HookReceiveListener();

	bool HasBreakPoint(FString FilePath, int32 Line);

	void SendEnterDebug(FString FilePath, int32 Line);

	void hook_call_option();

	void hook_ret_option();

	void hook_line_option();

public:

	UPROPERTY(Config)
		FString HookIP;

	UPROPERTY(Config)
		int32 HookPort;

protected:

	void RegisterLuaState(lua_State* State);

	void UnRegisterLuaState(bool bFullCleanup);

	//void HookTick(float DeltaTimes);

	//Rama's StringFromBinaryArray
	FString StringFromBinaryArray(TArray<uint8>& BinaryArray);

	void ReceInitData(const uint8* BinaryPointer, uint32_t BinarySize);

	void ReceBreakPoints(const uint8* BinaryPointer, uint32_t BinarySize);

	void ReceReqStack(const uint8* BinaryPointer, uint32_t BinarySize);

	void ReceReqChild(const uint8* BinaryPointer, uint32_t BinarySize);

	void SendClientExit();

	void ReceContinue();

	void ReceStepOver();

	void ReceStepIn();

	void ReceStepOut();

	void GetStackVars(int32 StackIndex);

	void GetVarsChildren(FScriptDebuggerVarNode& InNode);

	bool NameTranslate(int32 KindType, FString& VarName, int32 StackIndex);

	void ValueTranslate(int32 KindType, FString& VarValue, FString& VarType, int32 StackIndex);

	void IteraionTable(FScriptDebuggerVarNode& InNode, int32 NameIndex);

	void LocalListen(FScriptDebuggerVarNode& InNode);

	void UpvalueListen(FScriptDebuggerVarNode& InNode);

	void GlobalListen(FScriptDebuggerVarNode& InNode);

	bool PropertyTranslate(FString& VarValue, FString& VarType, int32& KindType, void*& VarPtr, UProperty* Property, UObject* Object);

	void UEObjectListen(FScriptDebuggerVarNode& InNode);

protected:

	class FClassDesc* UEClassDesc;
	UObject* UEObject;
	static const FString TempVarName;
	static const FString SelfName;
	static const FString OverriddenName;
	static const FText ClassDescText;
	static const FText UFunctionText;

	//transform name
	static const FString SelfLocationName;
	static const FString SelfRotatorName;
	static const FString SelfScalerName;
	static const FString FVectorName;
	static const FString FRotatorName;
	static const FString ReturnValueName;

	//transform type
	static const FText SelfLocationText;
	static const FText SelfRotatorText;
	static const FText SelfScalerText;
	static const FText FVectorText;
	static const FText FRotatorText;

	struct lua_State *L;

	unlua_over u_over;

	unlua_out u_out;

	EHookMode hook_mode;

	FDelegateHandle RegLuaHandle;

	FDelegateHandle UnRegLuaHandle;

	TSharedPtr<FInternetAddr> ServerAddr;

	FIPv4Address ServerIp;

	FSocket* HostSocket;

	FRunnableThread* HookReceiveThread;

	bool IsRunning;

	TMap<FString, TSet<int32>> BreakPoints;

	FCriticalSection QueueCritical;

	TMap<FString, TSet<int32>> EnableBreakPoint;

	TArray<FScriptDebuggerVarNode_Ref> VarList;

	FString HostScriptPath;

	const static FString ScriptMask;

	//int32 DebugCount;

	//FDelegateHandle HookTickHandle;

	volatile bool IsLeaveDebug;

	TArray<uint8> ReceivedData;

	uint32 TotalSize = 0;
};
