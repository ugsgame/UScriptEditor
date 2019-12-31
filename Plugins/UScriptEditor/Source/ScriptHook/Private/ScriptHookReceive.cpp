// Fill out your copyright notice in the Description page of Project Settings.


#include "ScriptHookReceive.h"
#include "WindowsPlatformProcess.h"
#include "ScriptHookClient.h"


uint32 FHookReceiveThread::Run()
{
	while (StopTaskCounter.GetValue() == 0)
	{
		FPlatformProcess::Sleep(0.05f);
		if (UScriptHookClient::Get()->HookReceiveListener())
			StopTaskCounter.Increment();
	}
	return 0;
}

void FHookReceiveThread::Stop()
{
	StopTaskCounter.Increment();

	UE_LOG(LogTemp, Log, TEXT("Remote HookReceiveThread Stop"));
}
