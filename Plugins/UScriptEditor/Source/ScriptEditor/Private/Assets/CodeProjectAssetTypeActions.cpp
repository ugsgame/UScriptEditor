// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "CodeProjectAssetTypeActions.h"
#include "SourceProject.h"
#include "ScriptEditor.h"


#define LOCTEXT_NAMESPACE "CodeProjectAssetTypeActions"


FText FCodeProjectAssetTypeActions::GetName() const
{
	return LOCTEXT("CodeProjectActionsName", "Code Project");
}

FColor FCodeProjectAssetTypeActions::GetTypeColor() const
{
	return FColor(255, 255, 0);
}

UClass* FCodeProjectAssetTypeActions::GetSupportedClass() const
{
	return USourceProject::StaticClass();
}

void FCodeProjectAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		if (USourceProject* CodeProject = Cast<USourceProject>(*ObjIt))
		{
			//TSharedRef<FCodeProjectEditor> NewCodeProjectEditor(new FCodeProjectEditor());
			//NewCodeProjectEditor->InitCodeEditor(Mode, EditWithinLevelEditor, CodeProject);
		}
	}
}

uint32 FCodeProjectAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

#undef LOCTEXT_NAMESPACE
