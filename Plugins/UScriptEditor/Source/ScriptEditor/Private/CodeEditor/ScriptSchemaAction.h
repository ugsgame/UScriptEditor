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

	/** Class of node we want to create */
	UPROPERTY()
	FString CodeClip;

	FScriptSchemaAction()
		: FEdGraphSchemaAction()
	{}

	FScriptSchemaAction(FText InNodeCategory, FText InMenuDesc, FText InToolTip,FString InCodeClip, const int32 InGrouping = 0)
		: FEdGraphSchemaAction(MoveTemp(InNodeCategory), MoveTemp(InMenuDesc), MoveTemp(InToolTip), InGrouping)
		,CodeClip(InCodeClip)
	{}

};

UCLASS()
class  UScriptActionCollecter : public UObject
{
	GENERATED_BODY()
public:
	static UScriptActionCollecter* Get();

	void Reflash();

	TArray<TSharedPtr<FEdGraphSchemaAction>> GetScriptActions();
	TArray<TSharedPtr<FEdGraphSchemaAction>> GetLuaActions();

	FGraphActionListBuilderBase* GetScriptActionList();
	FGraphActionListBuilderBase* GetLuaActionList();
protected:
	void CreateScriptActions();
	void CreateLuaActions();

	void AddActionByClass(UClass* Class,bool CategoryByClass = false);

	void AddScriptAction(FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString CodeClip);
	void AddLuaAction(FString InNodeCategory, FString InMenuDesc, FString InToolTip, FString CodeClip);
	
	FString GetAPICodeClip(UClass *Class, UFunction *Function,bool WithNote = false)const;
private:

	TArray<TSharedPtr<FEdGraphSchemaAction>> ScriptActions;
	TArray<TSharedPtr<FEdGraphSchemaAction>> LuaActions;

	FGraphActionListBuilderBase* ScriptActionList;
	FGraphActionListBuilderBase* LuaActionList;
};
