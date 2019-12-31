#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScriptWidget.generated.h"

UCLASS()
class SCRIPTHELPER_API UScriptWidget :public UUserWidget
{
	GENERATED_BODY()
public:
	UScriptWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void PostLoad() override;
	virtual bool Initialize() override;

protected:
	UPROPERTY(/*VisibleAnywhere,*/ BlueprintReadOnly, EditDefaultsOnly, Category = "Script")
	class UScriptDataAsset* ScriptData;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Script")
	bool HasScriptBinding;
};

