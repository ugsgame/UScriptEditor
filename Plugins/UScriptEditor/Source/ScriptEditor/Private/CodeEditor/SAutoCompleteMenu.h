#pragma once

#include "CoreMinimal.h"
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

DECLARE_DELEGATE_OneParam(FOnActionCodeSelectedEvent, const FString &);

class SAutoCompleteMenu : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SAutoCompleteMenu)
		: _CodeEditableObj(static_cast<SCodeEditableText*>(NULL))
		, _NewNodePosition(FVector2D::ZeroVector)
		, _AutoExpandActionMenu(false){}

	SLATE_ARGUMENT(SCodeEditableText*, CodeEditableObj)
		SLATE_ARGUMENT(FVector2D, NewNodePosition)
		SLATE_ARGUMENT(bool, AutoExpandActionMenu)
		SLATE_ARGUMENT(FScriptReferenceInfo, ReferenceInfo)
		SLATE_ARGUMENT(ECompleteParseType, ParseType)
		SLATE_ARGUMENT(bool,SelfContext);

		SLATE_EVENT(FOnActionCodeSelectedEvent, OnActionCodeSelected)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FScriptEditor> InEditor);

	~SAutoCompleteMenu();

	TSharedRef<SEditableTextBox> GetFilterTextBox();
	void SetFilterText(FText InFilterText);
	void GetSelectedActions(TArray< TSharedPtr<FEdGraphSchemaAction> >& OutSelectedActions) const;
	bool IsMatchingAny();
	void SetUserFoucs(class FReply& Reply);
protected:
	void OnActionSelected(const TArray< TSharedPtr<FEdGraphSchemaAction> >& SelectedAction, ESelectInfo::Type InSelectionType);
	/** Callback used to populate all actions list in SGraphActionMenu */
	void CollectAllActions(FGraphActionListBuilderBase& OutAllActions);
	FText OnGetFilterText();
private:
	SCodeEditableText* CodeEditableObj;
	FVector2D NewNodePosition;
	bool bAutoExpandActionMenu;

	FOnActionCodeSelectedEvent OnActionCodeSelected;

	TSharedPtr<SGraphActionMenu> GraphActionMenu;
	TWeakPtr<FScriptEditor> EditorPtr;

	bool bActionExecuted;
	FText CurFilterText;

	FScriptReferenceInfo ReferenceInfo;
	ECompleteParseType ParseType;
	bool SelfContext;
};