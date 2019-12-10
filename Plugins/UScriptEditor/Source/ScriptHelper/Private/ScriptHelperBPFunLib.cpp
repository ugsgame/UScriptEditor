// Fill out your copyright notice in the Description page of Project Settings.

#include "ScriptHelperBPFunLib.h"
#include "Misc/Paths.h"

FString UScriptHelperBPFunLib::ScriptSourceRoot()
{
	return TEXT("Script/");
}

FString UScriptHelperBPFunLib::ScriptSourceDir()
{
	return FPaths::ProjectDir() + ScriptSourceRoot();
}
