#pragma once

#include "CoreMinimal.h"
#include "ScriptEditorType.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "Widgets/Layout/SBorder.h"
#include "EdGraph/EdGraphSchema.h"
#include "GraphEditor.h"

class FScriptEditor;
class SBlueprintContextTargetMenu;
class SEditableTextBox;
class SGraphActionMenu;
class SCodeEditor;
class SCodeEditableText;

struct FBlueprintActionContext;
struct FCustomExpanderData;

/*******************************************************************************
* SScriptActionMenu
*******************************************************************************/

class SScriptActionMenu : public SBorder
{
public:
	/** Delegate for the OnCloseReason event which is always raised when the SScriptActionMenu closes */
	DECLARE_DELEGATE_ThreeParams(FClosedReason, bool /*bActionExecuted*/, bool /*bContextSensitiveChecked*/, bool /*bGraphPinContext*/);

	SLATE_BEGIN_ARGS(SScriptActionMenu)
		: _CodeEditableObj(static_cast<SCodeEditableText*>(NULL))
		, _NewNodePosition(FVector2D::ZeroVector)
		, _AutoExpandActionMenu(false){}

		SLATE_ARGUMENT(SCodeEditableText*, CodeEditableObj)
		SLATE_ARGUMENT(FVector2D, NewNodePosition)
		//SLATE_ARGUMENT(TArray<UEdGraphPin*>, DraggedFromPins)
		//SLATE_ARGUMENT(SCodeEditor::FActionMenuClosed, OnClosedCallback)
		SLATE_ARGUMENT(bool, AutoExpandActionMenu)
		SLATE_ARGUMENT(FScriptReferenceInfo, ReferenceInfo)

		SLATE_EVENT(FClosedReason, OnCloseReason)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FScriptEditor> InEditor);

	~SScriptActionMenu();

	TSharedRef<SEditableTextBox> GetFilterTextBox();

protected:
	/** UI Callback functions */
	EVisibility GetTypeImageVisibility() const;
	FText GetSearchContextDesc() const;
	void OnContextToggleChanged(ECheckBoxState CheckState);
	ECheckBoxState ContextToggleIsChecked() const;
	void OnContextTargetsChanged(uint32 ContextTargetMask);

	void OnActionSelected(const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedAction, ESelectInfo::Type InSelectionType);

	//TSharedRef<SWidget> OnCreateWidgetForAction(struct FCreateWidgetForActionData* const InCreateData);


	/** Callback used to populate all actions list in SGraphActionMenu */
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);

	/**  */
	void ConstructActionContext(FBlueprintActionContext& ContextDescOut);

	/** Functioin to try to insert a promote to variable entry if it is possible to do so. */
	void TryInsertPromoteToVariable(FBlueprintActionContext const& Context, FGraphActionListBuilderBase& OutAllActions);

private:
	SCodeEditableText* CodeEditableObj;
	FVector2D NewNodePosition;
	bool bAutoExpandActionMenu;

	//SGraphEditor::FActionMenuClosed OnClosedCallback;
	FClosedReason OnCloseReasonCallback;

	TSharedPtr<SGraphActionMenu> GraphActionMenu;
	TWeakPtr<FScriptEditor> EditorPtr;
	//TSharedPtr<SBlueprintContextTargetMenu> ContextTargetSubMenu;

	FScriptReferenceInfo ReferenceInfo;

	bool bActionExecuted;
};

/*******************************************************************************
* SScriptActionMenuExpander
*******************************************************************************/

class SScriptActionMenuExpander : public SExpanderArrow
{
	SLATE_BEGIN_ARGS(SScriptActionMenuExpander) {}
		SLATE_ATTRIBUTE(float, IndentAmount)
	SLATE_END_ARGS()

public:
	/**
	 * Constructs a standard SExpanderArrow widget if the associated menu item
	 * is a category or separator, otherwise, for action items, it constructs
	 * a favoriting toggle (plus indent) in front of the action entry.
	 *
	 * @param  InArgs			A set of slate arguments for this widget type (defined above).
	 * @param  ActionMenuData	A set of useful data for detailing the specific action menu row this is for.
	 */
	void Construct(const FArguments& InArgs, const FCustomExpanderData& ActionMenuData);

private:
	/**
	 * Action menu expanders are also responsible for properly indenting the
	 * menu entries, so this returns the proper margin padding for the menu row
	 * (based off its indent level).
	 *
	 * @return Padding to construct around this widget (so the menu entry is properly indented).
	 */
	FMargin GetCustomIndentPadding() const;

	/** The action associated with the menu row this belongs to */
	TWeakPtr<FEdGraphSchemaAction> ActionPtr;
};