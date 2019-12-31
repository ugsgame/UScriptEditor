#pragma once

#include "CoreMinimal.h"
#include "ScriptAnimInstance.generated.h"

//TODO:Not work yet!!
UCLASS()
class SCRIPTHELPER_API UScriptAnimInstance :public UAnimInstance
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	virtual void NativeInitializeAnimation() override;
protected:
	UPROPERTY(EditAnywhere,BlueprintReadOnly, /*EditDefaultsOnly,*/ Category = "Script")
	class UScriptDataAsset* ScriptData;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Script")
	bool HasScriptBinding;
};
