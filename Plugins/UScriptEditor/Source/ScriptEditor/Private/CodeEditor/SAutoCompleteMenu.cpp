
#include "SAutoCompleteMenu.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "SGraphActionMenu.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "EditorStyleSet.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "EdGraphSchema_K2.h"
#include "Kismet/Private/SBlueprintPalette.h"
#include "BlueprintEditor.h"
#include "Kismet/Private/SMyBlueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionFilter.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintPaletteFavorites.h"
#include "IDocumentation.h"
#include "SSCSEditor.h"
#include "Kismet/Private/SBlueprintContextTargetMenu.h"

#include "ScriptEditor.h"
#include "SCodeEditor.h"
#include "SCodeEditableText.h"
#include "ScriptEditorStyle.h"
#include "ScriptSchemaAction.h"
#include "BlueprintActionDatabase.h"
#include "MetaData.h"

#include "SScriptActionMenu.h"

#define LOCTEXT_NAMESPACE "SAutoCompleteMenu"


SAutoCompleteMenu::~SAutoCompleteMenu()
{

}

void SAutoCompleteMenu::Construct(const FArguments& InArgs, TSharedPtr<FScriptEditor> InEditor)
{
	bActionExecuted = false;

	this->CodeEditableObj = InArgs._CodeEditableObj;
	this->NewNodePosition = InArgs._NewNodePosition;
	this->bAutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	this->OnActionCodeSelected = InArgs._OnActionCodeSelected;
	this->EditorPtr = InEditor;


	// Build the widget layout
	SBorder::Construct(SBorder::FArguments()
		.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
		//.BorderImage(FScriptEditorStyle::Get().GetBrush("ProjectEditor.Border"))
		.Padding(5)
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(180)
			.HeightOverride(200)
			[
				SNew(SVerticalBox)
				// TYPE OF SEARCH INDICATOR
				// ACTION LIST 
				+ SVerticalBox::Slot()
				[
					SAssignNew(GraphActionMenu, SGraphActionMenu)
					.OnActionSelected(this, &SAutoCompleteMenu::OnActionSelected)
					.OnCollectAllActions(this, &SAutoCompleteMenu::CollectAllActions)
					//.OnGetFilterText(this,&SAutoCompleteMenu::OnGetFilterText)
				]
			]
		]
	);

}

TSharedRef<SEditableTextBox> SAutoCompleteMenu::GetFilterTextBox()
{
	return GraphActionMenu->GetFilterTextBox();
}

void SAutoCompleteMenu::SetFilterText(FText InFilterText)
{
	CurFilterText = InFilterText;
//	US_Log("InFilterText:%s", *(CurFilterText.ToString()));
	if (GraphActionMenu.IsValid())
	{
		TSharedPtr<SEditableTextBox> TextBox = GraphActionMenu->GetFilterTextBox();
		if (TextBox.IsValid())
		{
			TextBox->SetText(InFilterText);
		}
	}

}


void SAutoCompleteMenu::GetSelectedActions(TArray< TSharedPtr<FEdGraphSchemaAction> >& OutSelectedActions) const
{
	GraphActionMenu->GetSelectedActions(OutSelectedActions);
}

bool SAutoCompleteMenu::IsMatchingAny()
{
	TArray < TSharedPtr<FEdGraphSchemaAction>> SelectedActions;
	GraphActionMenu->GetSelectedActions(SelectedActions);
	return SelectedActions.Num()>0;
}


void SAutoCompleteMenu::SetUserFoucs(FReply& Reply)
{
	Reply.SetUserFocus(GraphActionMenu.ToSharedRef(), EFocusCause::SetDirectly);
}

void SAutoCompleteMenu::OnActionSelected(const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedAction, ESelectInfo::Type InSelectionType)
{
	if (InSelectionType == ESelectInfo::OnMouseClick || InSelectionType == ESelectInfo::OnKeyPress || SelectedAction.Num() == 0)
	{
		for (int32 ActionIndex = 0; ActionIndex < SelectedAction.Num(); ActionIndex++)
		{
			if (SelectedAction[ActionIndex].IsValid())
			{
				// Don't dismiss when clicking on dummy action
				if (SelectedAction[ActionIndex]->GetTypeId() != FEdGraphSchemaAction_Dummy::StaticGetTypeId())
				{
					FScriptSchemaAction* ScriptAction = static_cast<FScriptSchemaAction*>(SelectedAction[ActionIndex].Get());
					//TODO:Add script code to the CodeEditor
					if (this->CodeEditableObj && ScriptAction)
					{
						OnActionCodeSelected.ExecuteIfBound(ScriptAction->CodeClip);
					}
					FSlateApplication::Get().DismissAllMenus();
				}
			}
		}
	}
}

void SAutoCompleteMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{

	if (UScriptActionCollecter::Get())
	{
		//UScriptActionCollecter::Get()->Reflash();

		for (TSharedPtr<FEdGraphSchemaAction> Action : UScriptActionCollecter::Get()->GetScriptActions())
		{
			OutAllActions.AddAction(Action);
		}

		for (TSharedPtr<FEdGraphSchemaAction> Action : UScriptActionCollecter::Get()->GetLuaActions())
		{
			OutAllActions.AddAction(Action);
		}
		/*
		UScriptActionCollecter::Get()->Reflash();
		OutAllActions.Append(*UScriptActionCollecter::Get()->GetScriptActionList());
		OutAllActions.Append(*UScriptActionCollecter::Get()->GetLuaActionList());
		*/
	}
}

FText SAutoCompleteMenu::OnGetFilterText()
{
	//US_Log("FilterText:%s", *(CurFilterText.ToString()));
	return CurFilterText;
}

#undef LOCTEXT_NAMESPACE