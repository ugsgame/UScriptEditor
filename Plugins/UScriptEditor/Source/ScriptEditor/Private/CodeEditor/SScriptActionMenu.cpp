
#include "SScriptActionMenu.h"
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

#define LOCTEXT_NAMESPACE "SScriptActionMenu"

/** Action to promote a pin to a variable */
USTRUCT()
struct FBlueprintAction_PromoteVariable : public FEdGraphSchemaAction
{
	FBlueprintAction_PromoteVariable(bool bInToMemberVariable)
		: FEdGraphSchemaAction(FText(),
			bInToMemberVariable ? LOCTEXT("PromoteToVariable", "Promote to variable") : LOCTEXT("PromoteToLocalVariable", "Promote to local variable"),
			bInToMemberVariable ? LOCTEXT("PromoteToVariable", "Promote to variable") : LOCTEXT("PromoteToLocalVariable", "Promote to local variable"),
			1)
		, bToMemberVariable(bInToMemberVariable)
	{
	}

	// FEdGraphSchemaAction interface
	virtual UEdGraphNode* PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override
	{
		if ((ParentGraph != NULL) && (FromPin != NULL))
		{
			UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(ParentGraph);
			if ((MyBlueprintEditor.IsValid() == true) && (Blueprint != NULL))
			{
				MyBlueprintEditor.Pin()->DoPromoteToVariable(Blueprint, FromPin, bToMemberVariable);
			}
		}
		return NULL;
	}
	// End of FEdGraphSchemaAction interface

	/* Pointer to the blueprint editor containing the blueprint in which we will promote the variable. */
	TWeakPtr<class FBlueprintEditor> MyBlueprintEditor;

	/* TRUE if promoting to member variable, FALSE if promoting to local variable */
	bool bToMemberVariable;
};

/**
 * Static method for binding with delegates. Spawns an instance of the custom
 * expander.
 *
 * @param  ActionMenuData	A set of useful data for detailing the specific action menu row this is for.
 * @return A new widget, intended to lead entries in an SGraphActionMenu.
 */
static TSharedRef<SExpanderArrow> CreateCustomBlueprintActionExpander(const FCustomExpanderData& ActionMenuData)
{
	return SNew(SScriptActionMenuExpander, ActionMenuData);
}

/*******************************************************************************
* SBlueprintActionFavoriteToggle
*******************************************************************************/

class SBlueprintActionFavoriteToggle : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SBlueprintActionFavoriteToggle) {}
	SLATE_END_ARGS()

public:
	/**
	 * Constructs a favorite-toggle widget (so that user can easily modify the
	 * item's favorited state).
	 *
	 * @param  InArgs			A set of slate arguments, defined above.
	 * @param  ActionPtrIn		The FEdGraphSchemaAction that the parent item represents.
	 * @param  BlueprintEdPtrIn	A pointer to the blueprint editor that the palette belongs to.
	 */
	void Construct(const FArguments& InArgs, const FCustomExpanderData& CustomExpanderData)
	{
		Container = CustomExpanderData.WidgetContainer;
		ActionPtr = CustomExpanderData.RowAction;

		ChildSlot
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Center)
			.FillWidth(1.0)
			[
				SNew(SCheckBox)
				.Visibility(this, &SBlueprintActionFavoriteToggle::IsVisibile)
			.ToolTipText(this, &SBlueprintActionFavoriteToggle::GetToolTipText)
			.IsChecked(this, &SBlueprintActionFavoriteToggle::GetFavoritedState)
			.OnCheckStateChanged(this, &SBlueprintActionFavoriteToggle::OnFavoriteToggled)
			.Style(FEditorStyle::Get(), "Kismet.Palette.FavoriteToggleStyle")
			]
			];
	}

private:
	/**
	 * Used to determine the toggle's visibility (this is only visible when the
	 * owning item is being hovered over, and the associated action can be favorited).
	 *
	 * @return True if this toggle switch should be showing, false if not.
	 */
	EVisibility IsVisibile() const
	{
		bool bNoFavorites = false;
		GConfig->GetBool(TEXT("BlueprintEditor.Palette"), TEXT("bUseLegacyLayout"), bNoFavorites, GEditorIni);

		UBlueprintPaletteFavorites const* const BlueprintFavorites = GetDefault<UEditorPerProjectUserSettings>()->BlueprintFavorites;

		EVisibility CurrentVisibility = EVisibility::Hidden;
		if (!bNoFavorites && BlueprintFavorites && BlueprintFavorites->CanBeFavorited(ActionPtr.Pin()))
		{
			if (BlueprintFavorites->IsFavorited(ActionPtr.Pin()) || Container->IsHovered())
			{
				CurrentVisibility = EVisibility::Visible;
			}
		}

		return CurrentVisibility;
	}

	/**
	 * Retrieves tooltip that describes the current favorited state of the
	 * associated action.
	 *
	 * @return Text describing what this toggle will do when you click on it.
	 */
	FText GetToolTipText() const
	{
		if (GetFavoritedState() == ECheckBoxState::Checked)
		{
			return LOCTEXT("Unfavorite", "Click to remove this item from your favorites.");
		}
		return LOCTEXT("Favorite", "Click to add this item to your favorites.");
	}

	/**
	 * Checks on the associated action's favorite state, and returns a
	 * corresponding checkbox state to match.
	 *
	 * @return ECheckBoxState::Checked if the associated action is already favorited, ECheckBoxState::Unchecked if not.
	 */
	ECheckBoxState GetFavoritedState() const
	{
		ECheckBoxState FavoriteState = ECheckBoxState::Unchecked;
		if (ActionPtr.IsValid())
		{
			const UEditorPerProjectUserSettings& EditorSettings = *GetDefault<UEditorPerProjectUserSettings>();
			if (UBlueprintPaletteFavorites* BlueprintFavorites = EditorSettings.BlueprintFavorites)
			{
				FavoriteState = BlueprintFavorites->IsFavorited(ActionPtr.Pin()) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
			}
		}
		return FavoriteState;
	}

	/**
	 * Triggers when the user clicks this toggle, adds or removes the associated
	 * action to the user's favorites.
	 *
	 * @param  InNewState	The new state that the user set the checkbox to.
	 */
	void OnFavoriteToggled(ECheckBoxState InNewState)
	{
		if (InNewState == ECheckBoxState::Checked)
		{
			GetMutableDefault<UEditorPerProjectUserSettings>()->BlueprintFavorites->AddFavorite(ActionPtr.Pin());
		}
		else
		{
			GetMutableDefault<UEditorPerProjectUserSettings>()->BlueprintFavorites->RemoveFavorite(ActionPtr.Pin());
		}
	}

private:
	/** The action that the owning palette entry represents */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;

	/** The widget that this widget is nested inside */
	TSharedPtr<SPanel> Container;
};

/*******************************************************************************
* SBlueprintActionMenu
*******************************************************************************/
SScriptActionMenu::~SScriptActionMenu()
{

}

void SScriptActionMenu::Construct(const FArguments& InArgs, TSharedPtr<FScriptEditor> InEditor)
{
	bActionExecuted = false;

	this->CodeEditableObj = InArgs._CodeEditableObj;
	this->NewNodePosition = InArgs._NewNodePosition;
	//this->OnClosedCallback = InArgs._OnClosedCallback;
	this->bAutoExpandActionMenu = InArgs._AutoExpandActionMenu;
	this->ReferenceInfo = InArgs._ReferenceInfo;
	this->EditorPtr = InEditor;
	this->OnCloseReasonCallback = InArgs._OnCloseReason;

	FSlateColor TypeColor;
	FString TypeOfDisplay;
	const FSlateBrush* ContextIcon = nullptr;

	/*
	if (DraggedFromPins.Num() == 1)
	{
		UEdGraphPin* OnePin = DraggedFromPins[0];

		const UEdGraphSchema* Schema = OnePin->GetSchema();
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

		if (!Schema->IsA(UEdGraphSchema_K2::StaticClass()) || !K2Schema->IsExecPin(*OnePin))
		{
			// Get the type color and icon
			TypeColor = Schema->GetPinTypeColor(OnePin->PinType);
			ContextIcon = FEditorStyle::GetBrush(OnePin->PinType.IsArray() ? TEXT("Graph.ArrayPin.Connected") : TEXT("Graph.Pin.Connected"));
		}
	}
	*/

	FBlueprintActionContext MenuContext;
	ConstructActionContext(MenuContext);

	/*
	TSharedPtr<SComboButton> TargetContextSubMenuButton;
	// @TODO: would be nice if we could use a checkbox style for this, and have a different state for open/closed
	SAssignNew(TargetContextSubMenuButton, SComboButton)
		.MenuPlacement(MenuPlacement_MenuRight)
		.HasDownArrow(false)
		.ButtonStyle(FEditorStyle::Get(), "BlueprintEditor.ContextMenu.TargetsButton")
		.MenuContent()
		[
			SAssignNew(ContextTargetSubMenu, SBlueprintContextTargetMenu, MenuContext)
			.OnTargetMaskChanged(this, &SScriptActionMenu::OnContextTargetsChanged)
		];
	*/

	// Build the widget layout
	SBorder::Construct(SBorder::FArguments()
		.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
		//.BorderImage(FScriptEditorStyle::Get().GetBrush("ProjectEditor.Border"))
		.Padding(5)
		[
			// Achieving fixed width by nesting items within a fixed width box.
			SNew(SBox)
			.WidthOverride(400)
			.HeightOverride(400)
			[
				SNew(SVerticalBox)

				// TYPE OF SEARCH INDICATOR
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(2, 2, 2, 5)
				[
					SNew(SHorizontalBox)

					// Type pill
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, (ContextIcon != NULL) ? 5 : 0, 0)
					[
						SNew(SImage)
						.ColorAndOpacity(TypeColor)
						.Visibility(this, &SScriptActionMenu::GetTypeImageVisibility)
						.Image(ContextIcon)
					]

					// Search context description
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SScriptActionMenu::GetSearchContextDesc)
						.Font(FEditorStyle::GetFontStyle(FName("BlueprintEditor.ActionMenu.ContextDescriptionFont")))
						.ToolTip(IDocumentation::Get()->CreateToolTip(
						LOCTEXT("BlueprintActionMenuContextTextTooltip", "Describes the current context of the action list"),
						NULL,
						TEXT("Shared/Editors/BlueprintEditor"),
						TEXT("BlueprintActionMenuContextText")))
						.WrapTextAt(280)
					]

					// Context Toggle
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &SScriptActionMenu::OnContextToggleChanged)
						.IsChecked(this, &SScriptActionMenu::ContextToggleIsChecked)
						.ToolTip(IDocumentation::Get()->CreateToolTip(
						LOCTEXT("BlueprintActionMenuContextToggleTooltip", "Should the list be filtered to only actions that make sense in the current context?"),
						NULL,
						TEXT("Shared/Editors/BlueprintEditor"),
						TEXT("BlueprintActionMenuContextToggle")))
						[
							SNew(STextBlock)
							.Text(LOCTEXT("BlueprintActionMenuContextToggle", "Context Sensitive"))
						]
					]

// 					+ SHorizontalBox::Slot()
// 					.HAlign(HAlign_Right)
// 					.VAlign(VAlign_Center)
// 					.AutoWidth()
// 					.Padding(3.f, 0.f, 0.f, 0.f)
// 					[
// 						TargetContextSubMenuButton.ToSharedRef()
// 					]
				]

				// ACTION LIST 
				+ SVerticalBox::Slot()
				[
					SAssignNew(GraphActionMenu, SGraphActionMenu)
					.OnActionSelected(this, &SScriptActionMenu::OnActionSelected)
					//.OnCreateWidgetForAction(SGraphActionMenu::FOnCreateWidgetForAction::CreateSP(this, &SScriptActionMenu::OnCreateWidgetForAction))
					.OnCollectAllActions(this, &SScriptActionMenu::CollectAllActions)
					.OnCreateCustomRowExpander_Static(&CreateCustomBlueprintActionExpander)
				]
			]
		]
	);
}

TSharedRef<SEditableTextBox> SScriptActionMenu::GetFilterTextBox()
{
	return GraphActionMenu->GetFilterTextBox();
}

EVisibility SScriptActionMenu::GetTypeImageVisibility() const
{
	//TODO:
	return EVisibility::Collapsed;
}

FText SScriptActionMenu::GetSearchContextDesc() const
{
	//TODO:
	return LOCTEXT("MenuPrompt_AllPins", "All Possible Actions");
}

void SScriptActionMenu::OnContextToggleChanged(ECheckBoxState CheckState)
{
	//TODO:
	GraphActionMenu->RefreshAllActions(true, false);
}

ECheckBoxState SScriptActionMenu::ContextToggleIsChecked() const
{
	//TODO:
	return ECheckBoxState::Unchecked;
}

void SScriptActionMenu::OnContextTargetsChanged(uint32 ContextTargetMask)
{
	//TODO:
	GraphActionMenu->RefreshAllActions(/*bPreserveExpansion =*/true, /*bHandleOnSelectionEvent =*/false);
}

void SScriptActionMenu::OnActionSelected(const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedAction, ESelectInfo::Type InSelectionType)
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
					FSlateApplication::Get().DismissAllMenus();
					FScriptSchemaAction* ScriptAction = static_cast<FScriptSchemaAction*>(SelectedAction[ActionIndex].Get());
					//TODO:Add script code to the CodeEditor
					if (this->CodeEditableObj && ScriptAction)
					{
						CodeEditableObj->InsertTextAtCursor(ScriptAction->CodeClip);
					}
				}
			}
		}
	}
}

// TSharedRef<SWidget> SScriptActionMenu::OnCreateWidgetForAction(struct FCreateWidgetForActionData* const InCreateData)
// {
// 	//TODO:
// 	InCreateData->bHandleMouseButtonDown = true;
// 	return SNew(SBlueprintPaletteItem, InCreateData, EditorPtr.Pin());
// }

void SScriptActionMenu::CollectAllActions(FGraphActionListBuilderBase& OutAllActions)
{
	//TODO:
	US_Log("CollectAllActions");

	if (UScriptActionCollecter::Get())
	{
		//UScriptActionCollecter::Get()->Reflash();

		for (TSharedPtr<FEdGraphSchemaAction> Action:UScriptActionCollecter::Get()->GetScriptActions())
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

void SScriptActionMenu::ConstructActionContext(FBlueprintActionContext& ContextDescOut)
{
	//TODO:
}

void SScriptActionMenu::TryInsertPromoteToVariable(FBlueprintActionContext const& Context, FGraphActionListBuilderBase& OutAllActions)
{
	//TODO:
}

/*******************************************************************************
* SScriptActionMenuExpander
*******************************************************************************/

void SScriptActionMenuExpander::Construct(const FArguments& InArgs, const FCustomExpanderData& ActionMenuData)
{
	OwnerRowPtr = ActionMenuData.TableRow;
	IndentAmount = InArgs._IndentAmount;
	ActionPtr = ActionMenuData.RowAction;

	if (!ActionPtr.IsValid())
	{
		SExpanderArrow::FArguments SuperArgs;
		SuperArgs._IndentAmount = InArgs._IndentAmount;

		SExpanderArrow::Construct(SuperArgs, ActionMenuData.TableRow);
	}
	else
	{
		ChildSlot
			.Padding(TAttribute<FMargin>(this, &SScriptActionMenuExpander::GetCustomIndentPadding))
			[
				SNew(SBlueprintActionFavoriteToggle, ActionMenuData)
			];
	}
}

FMargin SScriptActionMenuExpander::GetCustomIndentPadding() const
{
	FMargin CustomPadding = SExpanderArrow::GetExpanderPadding();
	// if this is a action row (not a category or separator)
	if (ActionPtr.IsValid())
	{
		// flip the left/right margins (we want the favorite toggle aligned to the far left)
		//CustomPadding = FMargin(CustomPadding.Right, CustomPadding.Top, CustomPadding.Left, CustomPadding.Bottom);
	}
	return CustomPadding;
}

#undef LOCTEXT_NAMESPACE