
#include "ScriptParamCollection.h"
#include "CoreUObject.h"

TArray<FString> GDefaultClassCollection;
TMap<FName, FFunctionCollection> GDefaultParamCollection;

#pragma optimize("", off)

void CreateDefaultParamCollection()
{
	static bool CollectionCreated = false;
	if (!CollectionCreated)
	{
		CollectionCreated = true;

//#include "UScriptParamCollection.inl"
 //#include "UScriptClassCollection.inl"	
	}
}

#pragma optimize("", on)

void DestroyDefaultParamCollection()
{
	for (TMap<FName, FFunctionCollection>::TIterator FCIt(GDefaultParamCollection); FCIt; ++FCIt)
	{
		FFunctionCollection &FunctionCollection = FCIt.Value();
		for (TMap<FName, FParameterCollection>::TIterator PCIt(FunctionCollection.Functions); PCIt; ++PCIt)
		{
			FParameterCollection &ParamCollection = PCIt.Value();
			for (TMap<FName, IParamValue*>::TIterator PVIt(ParamCollection.Parameters); PVIt; ++PVIt)
			{
				IParamValue *ParamValue = PVIt.Value();
				delete ParamValue;
			}
			ParamCollection.Parameters.Empty();
		}
		FunctionCollection.Functions.Empty();
	}
	GDefaultParamCollection.Empty();
	GDefaultClassCollection.Empty();
}