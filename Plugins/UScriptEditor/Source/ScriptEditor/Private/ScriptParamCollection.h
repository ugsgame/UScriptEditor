#pragma once
#include "CoreMinimal.h"

enum EScriptParamType
{
	Bool,
	Byte,
	Int,
	Enum,
	Float,
	Double,
	Name,
	Text,
	String,
	Vector,
	Vector2D,
	Rotator,
	LinearColor,
	Color,
	Array,
	Object,
	Unknow,
};

class IParamValue
{
public:
	virtual ~IParamValue() {}

	virtual const void* GetValue() const = 0;
	virtual EScriptParamType GetType()const = 0;
};

template <typename T>
class TParamValue : public IParamValue
{
public:
	explicit TParamValue(const T &InValue, EScriptParamType InType = EScriptParamType::Unknow)
		: Value(InValue), Type(InType)
	{}

	virtual const void* GetValue() const override { return &Value; }
	virtual EScriptParamType GetType() const override { return Type; }

private:
	T Value;
	EScriptParamType Type;
};

typedef TParamValue<bool> FBoolParamValue;
typedef TParamValue<uint8> FByteParamValue;
typedef TParamValue<int32> FIntParamValue;
typedef TParamValue<int64> FEnumParamValue;
typedef TParamValue<float> FFloatParamValue;
typedef TParamValue<double> FDoubleParamValue;
typedef TParamValue<FName> FNameParamValue;
typedef TParamValue<FText> FTextParamValue;
typedef TParamValue<FString> FStringParamValue;
typedef TParamValue<FVector> FVectorParamValue;
typedef TParamValue<FVector2D> FVector2DParamValue;
typedef TParamValue<FRotator> FRotatorParamValue;
typedef TParamValue<FLinearColor> FLinearColorParamValue;
typedef TParamValue<FColor> FColorParamValue;
//
typedef TParamValue<FString> FTArrayParamValue;
typedef TParamValue<FString> FUObjectParamValue;
typedef TParamValue<FString> FUnknowParamValue;

struct FParameterCollection
{
	FString Category;
	FString ToolTip;
	TMap<FName, IParamValue*> Parameters;
};

struct FFunctionCollection
{
	TMap<FName, FParameterCollection> Functions;
};

extern TArray<FString> GDefaultClassCollection;

extern TMap<FName, FFunctionCollection> GDefaultParamCollection;

extern void CreateDefaultParamCollection();
extern void DestroyDefaultParamCollection();