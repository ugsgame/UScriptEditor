#pragma once

UENUM(BlueprintType)
enum class EScriptTemplateType : uint8
{
	ActorComponent = 0,
	Actor = 1,
	AnimInstance = 2,
	UserWidget = 3,
};
