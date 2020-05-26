// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

/**
 * 
 */
class SCRIPTHOOK_API FHookReceiveThread : public FRunnable
{
public:

	FThreadSafeCounter StopTaskCounter;

public:
	FHookReceiveThread() : StopTaskCounter(0) {}

	virtual uint32 Run() override;

	virtual void Stop() override;
};
