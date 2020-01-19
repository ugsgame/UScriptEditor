#pragma once


#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "AssetData.h"
#include "EdGraph/EdGraphSchema.h"

#include "ScriptSchemaAction.generated.h"

/** Action to add a script function to the CodeEditor */
USTRUCT()
struct  FScriptSchemaAction : public FEdGraphSchemaAction
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY()
	UClass* Class;
	/** Class of node we want to create */
	UPROPERTY()
	FString CodeClip;

	UPROPERTY()
	bool IsStatic;

	UPROPERTY()
	bool IsFunction;

	FScriptSchemaAction()
		: FEdGraphSchemaAction()
	{}

	FScriptSchemaAction(FText InNodeCategory, FText InMenuDesc, FText InToolTip,FString InCodeClip,bool InIsStatic,bool InIsFuncion, const int32 InGrouping = 0)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
		, CodeClip(InCodeClip)
		, IsStatic(InIsStatic)
		, IsFunction(InIsFuncion)
	{}

};

UCLASS()
class  UScriptActionCollecter : public UObject
{
	GENERATED_BODY()
public:
	static UScriptActionCollecter* Get();

	void Reflash();

	TArray<TSharedPtr<FScriptSchemaAction>> GetScriptActions();
	TArray<TSharedPtr<FScriptSchemaAction>> GetLuaActions();

	FGraphActionListBuilderBase* GetScriptActionList();
	FGraphActionListBuilderBase* GetLuaActionList();
protected:
	void CreateScriptActions();
	void CreateLuaActions();

	void AddActionByClass(UClass* Class,bool CategoryByClass = false);

	void AddScriptAction(UClass* Class,bool InIsStatic,bool InIsFunction,FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString InCodeClip);
	void AddLuaAction(FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString InCodeClip);
	
	FString GetAPICodeClip(UClass *Class, UFunction *Function,bool WithNote = false)const;
private:

	TArray<TSharedPtr<FScriptSchemaAction>> ScriptActions;

	TArray<TSharedPtr<FScriptSchemaAction>> ScriptFunctions;
	TArray<TSharedPtr<FScriptSchemaAction>> ScriptStaticFunctions;
	TArray<TSharedPtr<FScriptSchemaAction>> ScriptVariables;
	TArray<TSharedPtr<FScriptSchemaAction>> ScriptStaticVariables;

	TArray<TSharedPtr<FScriptSchemaAction>> LuaActions;

	FGraphActionListBuilderBase* ScriptActionList;
	FGraphActionListBuilderBase* LuaActionList;
};
