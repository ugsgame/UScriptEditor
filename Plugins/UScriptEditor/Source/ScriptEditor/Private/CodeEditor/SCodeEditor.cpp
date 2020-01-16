// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SCodeEditor.h"
#include "Misc/FileHelper.h"
#include "Framework/Text/TextLayout.h"

#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SEditableText.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "ScriptEditorStyle.h"
#include "CodeProjectItem.h"
#include "ScriptEditorUtils.h"
#include "CPPRichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "LUARichTextSyntaxHighlighterTextLayoutMarshaller.h"
#include "SCodeEditableText.h"

#include "ScriptEditor.h"
#include "SScriptDebugger.h"
#include "ScriptEditorSetting.h"

#include "Editor/EditorStyle/Public/EditorStyle.h"
#include "AssetRegistryModule.h"
#include "AssetRegistryInterface.h"

#define LOCTEXT_NAMESPACE "CodeEditor"


void SCodeEditor::Construct(const FArguments& InArgs, UCodeProjectItem* InCodeProjectItem)
{
	bDirty = false;

	check(InCodeProjectItem);
	CodeProjectItem = InCodeProjectItem;

	FString FileText = "File Loading, please wait";
	FFileHelper::LoadFileToString(FileText, *InCodeProjectItem->Path);

	TSharedPtr<FSyntaxHighlighterTextLayoutMarshaller> RichTextMarshaller = nullptr;

	TSharedRef<FCPPRichTextSyntaxHighlighterTextLayoutMarshaller> CPPRichTextMarshaller = FCPPRichTextSyntaxHighlighterTextLayoutMarshaller::Create(
		FCPPRichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle()
	);

	TSharedRef<FLUARichTextSyntaxHighlighterTextLayoutMarshaller> LUARichTextMarshaller = FLUARichTextSyntaxHighlighterTextLayoutMarshaller::Create(
		FLUARichTextSyntaxHighlighterTextLayoutMarshaller::FSyntaxTextStyle()
	);
	//TODO:后面要不后缀名对比
	if (CodeProjectItem->Extension == "lua")
	{
		RichTextMarshaller = LUARichTextMarshaller;
	}
	else
	{
		RichTextMarshaller = CPPRichTextMarshaller;
	}
	//

	HorizontalScrollbar =
		SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(14.0f, 14.0f));

	VerticalScrollbar =
		SNew(SScrollBar)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(14.0f, 14.0f));

	/*
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SGridPanel)
			.FillColumn(0, 1.0f)
			.FillRow(0, 1.0f)
			+SGridPanel::Slot(0, 0)
			[
				SAssignNew(CodeEditableText, SCodeEditableText)
				.OnTextChanged(this, &SCodeEditor::OnTextChanged)
				.OnTextCommitted(this, &SCodeEditor::OnTextCommitted)
				.Text(FText::FromString(FileText))
				.Marshaller(RichTextMarshaller)
				.HScrollBar(HorizontalScrollbar)
				.VScrollBar(VerticalScrollbar)
			]
			+SGridPanel::Slot(1, 0)
			[
				VerticalScrollbar.ToSharedRef()
			]
			+SGridPanel::Slot(0, 1)
			[
				HorizontalScrollbar.ToSharedRef()
			]
		]
	];
	*/


	TSharedPtr<SOverlay>OverlayWidget; this->ChildSlot
		[

			SNew(SBorder)
			.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				SAssignNew(VS_SCROLL_BOX, SScrollBox)
				.OnUserScrolled(this, &SCodeEditor::OnVerticalScroll)
				.Orientation(EOrientation::Orient_Vertical)
				.ScrollBarThickness(FVector2D(8.f, 8.f))
				+ SScrollBox::Slot()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Fill).HAlign(HAlign_Left).AutoWidth()
					[
						SNew(SBorder)
						.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
						//.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						.BorderImage(FEditorStyle::GetBrush("NoBorder"))
						[
							SAssignNew(LineCounter, SListView<FCodeLineNode_Ptr>)
							.OnSelectionChanged(this, &SCodeEditor::OnSelectedLineCounterItem)
							.OnMouseButtonDoubleClick(this, &SCodeEditor::OnDoubleClickLineCounterItem)
							.OnGenerateRow(this, &SCodeEditor::OnGenerateLineCounter)
							.ScrollbarVisibility(EVisibility::Collapsed)
							.ListItemsSource(&LineCount).ItemHeight(14)
							.SelectionMode(ESelectionMode::Single)
						]
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Fill).HAlign(HAlign_Fill).AutoWidth()
					[
						SAssignNew(CodeEditableText, SCodeEditableText)
						.OnTextChanged(this, &SCodeEditor::OnTextChanged)
						.OnTextCommitted(this, &SCodeEditor::OnTextCommitted)
						.IsEnabled(this, &SCodeEditor::IsCodeEditable)			//TODO:Can not edit if debugging
						.OnInvokeSearch(this, &SCodeEditor::OnInvokedSearch)
						.OnAutoComplete(this, &SCodeEditor::OnAutoComplete)
						.Text(FText::FromString(FileText))
						.VScrollBar(VerticalScrollbar)
						.HScrollBar(HorizontalScrollbar)
						.Marshaller(RichTextMarshaller)
						.CanKeyboardFocus(true)
						.IsReadOnly(false)				//TODO:ReadOnly if debugging
					]
				]
			]
		];

	//Add Line Number
	SetLineCountList(GetLineCount());
}

void SCodeEditor::CheckReferences()
{
	if (CodeProjectItem && CodeProjectItem->ScriptDataAsset)
	{
		TArray<FAssetIdentifier> Identifiers;
		FName PackageName = CodeProjectItem->ScriptDataAsset->GetOutermost()->GetFName();
		Identifiers.Add(FAssetIdentifier(PackageName));

		//US_Log("PackageName:%s", *PackageName.ToString());

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetIdentifier> ReferenceNames;

		ReferenceInfoes.Empty();

		for (const FAssetIdentifier& AssetId : Identifiers)
		{
			AssetRegistryModule.Get().GetReferencers(AssetId, ReferenceNames, EAssetRegistryDependencyType::Packages);//true		
		}

		for (const FAssetIdentifier& reference : ReferenceNames)
		{
			US_Log("Reference:%s", *reference.PackageName.ToString());

			TArray<FAssetData> AssetDatas;
			AssetRegistryModule.Get().GetAssetsByPackageName(reference.PackageName, AssetDatas);

			for (FAssetData Asset : AssetDatas)
			{
				FScriptReferenceInfo ReferenceInfo;
				ReferenceInfo.ReferencedAsset = Asset.AssetName;
				//Blueprint Class
				if (Asset.GetClass()->IsChildOf(UBlueprint::StaticClass()))
				{				
					UBlueprint* BlueprintAsset = Cast<UBlueprint>(Asset.FastGetAsset());
					if (BlueprintAsset)
					{
						UClass* Class = BlueprintAsset->ParentClass;						
						TArray<UBlueprint*> OutBlueprintParents;

						GetBlueprintClassParents(Class, OutBlueprintParents);
						OutBlueprintParents.Insert(BlueprintAsset, 0);

						for (UBlueprint* bp:OutBlueprintParents)
						{
							US_Log("BlueprintClass:%s", *bp->GetName());
						}

						TArray<UClass*> OutNativeParents;
						GetNativeClassParents(OutBlueprintParents[OutBlueprintParents.Num()-1]->ParentClass, OutNativeParents);

						for (UClass* klass : OutNativeParents)
						{
							US_Log("NativeClass:%s", *klass->GetName());
						}	

						ReferenceInfo.BlueprintClasses = OutBlueprintParents;
						ReferenceInfo.NativeClasses = OutNativeParents;
					}
				}
				//Native Class
				else
				{
					TArray<UClass*> OutNativeParents;
					GetNativeClassParents(Asset.GetClass(), OutNativeParents);

					for (UClass* klass : OutNativeParents)
					{
						US_Log("NativeClass:%s", *klass->GetName());
					}

					ReferenceInfo.NativeClasses = OutNativeParents;
				}

				ReferenceInfoes.Add(ReferenceInfo);
			}
		}
	}
}

bool SCodeEditor::GetBlueprintClassParents(const UClass* InClass, TArray<UBlueprint*>& OutBlueprintParents)
{
	OutBlueprintParents.Empty();

	bool bNoErrors = true;
	const UClass* CurrentClass = InClass;
	while (UBlueprint* BP = UBlueprint::GetBlueprintFromClass(CurrentClass))
	{
		OutBlueprintParents.Add(BP);

#if WITH_EDITORONLY_DATA
		bNoErrors &= (BP->Status != BS_Error);
#endif // #if WITH_EDITORONLY_DATA

		// If valid, use stored ParentClass rather than the actual UClass::GetSuperClass(); handles the case when the class has not been recompiled yet after a reparent operation.
		if (const UClass* ParentClass = BP->ParentClass)
		{
			CurrentClass = ParentClass;
		}
		else
		{
			check(CurrentClass);
			CurrentClass = CurrentClass->GetSuperClass();
		}
	}

	return bNoErrors;
}

bool SCodeEditor::GetNativeClassParents(const UClass* InClass, TArray<UClass*>& OutNativeClassParents)
{
	OutNativeClassParents.Empty();

	const UClass* CurrentClass = InClass;
	while (CurrentClass)
	{
		UClass* NativeClass = CurrentClass->GetSuperClass();
		if (NativeClass)
		{
			OutNativeClassParents.Add(NativeClass);
		}
		CurrentClass = NativeClass;
	}

	return true;
}

void SCodeEditor::OnInvokedSearch()
{

}

void SCodeEditor::OnAdvanceAutoComplete(const FString &Search)
{

}

void SCodeEditor::OnAutoComplete(const FString &Results)
{

}

void SCodeEditor::OnTextChanged(const FText& NewText)
{
	bDirty = true;

	SetLineCountList(GetLineCount());

	//Sync to the ScriptAsset?

	if (CodeProjectItem->ScriptDataAsset)
	{
		FString CodeText = CodeEditableText->GetText().ToString();
		CodeProjectItem->ScriptDataAsset->SourceCode = CodeText;
		//TODO:Should be UTF-8 to bytes
		ScriptEditorUtils::StringToByteArray(CodeText, CodeProjectItem->ScriptDataAsset->ByteCode);
		//

		//Set Asset To Dirty
		CodeProjectItem->ScriptDataAsset->MarkPackageDirty();
	}

	//
}

void SCodeEditor::OnTextCommitted(const FText &NewText, ETextCommit::Type ComtInfo)
{
	SetLineCountList(GetLineCount());
}

void SCodeEditor::OnVerticalScroll(float Offset)
{
	VerticalScrollbar->SetState(VS_SCROLL_BOX->GetScrollOffset(), VS_SCROLL_BOX->GetViewOffsetFraction());
}

bool SCodeEditor::IsCodeEditable() const
{
	return true;
}

bool SCodeEditor::Save() const
{
	if (bDirty)
	{
		bool bResult = FFileHelper::SaveStringToFile(CodeEditableText->GetText().ToString(), *CodeProjectItem->Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		if (bResult)
		{
			bDirty = false;
			//Save to asset
			if (CodeProjectItem->ScriptDataAsset)
			{
				CodeProjectItem->ScriptDataAsset->SourceCode = CodeEditableText->GetText().ToString();
				FFileHelper::LoadFileToArray(CodeProjectItem->ScriptDataAsset->ByteCode, *CodeProjectItem->Path);
				//ScriptEditorUtils::StringToByteArray(CodeProjectItem->ScriptDataAsset->SourceCode, CodeProjectItem->ScriptDataAsset->ByteCode);

				ScriptEditorUtils::SaveScriptAsset(CodeProjectItem->ScriptDataAsset);
			}
		}

		return bResult;
	}
	return true;
}

bool SCodeEditor::CanSave() const
{
	return bDirty;
}

bool SCodeEditor::Reload()
{
	FString FileText = "File Loading, please wait";
	//
	CheckReferences();

	if (FFileHelper::LoadFileToString(FileText, *CodeProjectItem->Path))
	{
		CodeEditableText->SetText(FText::FromString(FileText));

		return true;
	}
	return false;
}

void SCodeEditor::Browser() const
{
	if (CodeProjectItem->ScriptDataAsset)
	{
		ScriptEditorUtils::BrowserToScriptAsset(CodeProjectItem->ScriptDataAsset);
	}
}

int32 SCodeEditor::GetLineCount() const
{
	TArray<FString>Array;
	int32 Count = 0;
	//
	FString Text = CodeEditableText->GetText().ToString();
	Count = Text.ParseIntoArray(Array, TEXT("\n"), false);
	//
	return Count;
}

UCodeProjectItem* SCodeEditor::GetCodeProjectItem() const
{
	return CodeProjectItem;
}

void SCodeEditor::GotoLineAndColumn(int32 LineNumber, int32 ColumnNumber)
{
	FSlateApplication::Get().SetKeyboardFocus(CodeEditableText.ToSharedRef());
	CodeEditableText->GoToLineColumn(LineNumber, ColumnNumber);
	CodeEditableText->SelectLine();

	LineNumber = LineNumber > 1 ? LineNumber + 1 : LineNumber;
	VS_SCROLL_BOX->SetScrollOffset((VS_SCROLL_BOX->GetScrollOffsetOfEnd() / GetLineCount())* LineNumber);
}

FText SCodeEditor::GetLineAndColumn() const
{
	int32 Line;
	int32 Column;
	CodeEditableText->GetLineAndColumn(Line, Column);

	FString LineAndColumn = FString::Printf(TEXT("Line: %d Column: %d"), Line + 1, Column);

	return FText::FromString(LineAndColumn);
}

void SCodeEditor::OnClose()
{
	//UScriptEdtiorSetting::Get()->EdittingFiles.Remove(CodeProjectItem->Path);

	//UE_LOG(LogTemp, Log, TEXT("CodeProjectItems [%d]"), UScriptEdtiorSetting::Get()->EdittingFiles.Num());
}

void SCodeEditor::SetLineCountList(const int32 Count)
{
	LineCount.Empty();
	//
	for (int32 I = 1; I <= Count; I++) {
		LineCount.Add(MakeShareable(new FCodeLineNode(I, CodeProjectItem->Path)));
	}///
	//
	if (LineCounter.IsValid()) { LineCounter->RequestListRefresh(); }
}

void SCodeEditor::OnSelectedLineCounterItem(FCodeLineNode_Ptr Item, ESelectInfo::Type SelectInfo)
{
	if (!Item.IsValid()) { return; }
	//
	const int32 LineID = Item->Line;
	//
	FSlateApplication::Get().SetKeyboardFocus(CodeEditableText.ToSharedRef());
	CodeEditableText->GoToLineColumn(LineID - 1, 0);
	CodeEditableText->SelectLine();
	//
	LineCounter->SetItemSelection(Item, false);
}

void SCodeEditor::OnDoubleClickLineCounterItem(FCodeLineNode_Ptr Item)
{
	if (!Item.IsValid()) { return; }
	//
	const int32 LineID = Item->Line;

	//TODO:Add or remove breakpoint
	//LineCounter->SetItemHighlighted(Item, !LineCounter->Private_IsItemHighlighted(Item));

	TSharedPtr<ITableRow> TableRow = LineCounter->WidgetFromItem(Item);
	//
}

TSharedRef<ITableRow> SCodeEditor::OnGenerateLineCounter(FCodeLineNode_Ptr Item, const TSharedRef<STableViewBase>&OwnerTable)
{
	Item->FilePath = CodeProjectItem->Path;

	return SNew(SCodeLineItem, OwnerTable, Item);
}

#undef LOCTEXT_NAMESPACE

void SCodeLineItem::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, FCodeLineNode_Ptr Line)
{
	CodeLine = Line;
	CodeLine->HasBreakPoint = FScriptEditor::Get()->HasBreakPoint(CodeLine->FilePath, CodeLine->Line);

	STableRow<FCodeLineNode_Ptr>::Construct(STableRow<FCodeLineNode_Ptr>::FArguments(), InOwnerTableView);
	ChildSlot
		[
			SNew(SBorder)
			//.BorderImage(FEditorStyle::GetBrush("Graph.Node.Body"))
		.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.f))
		.ForegroundColor(FSlateColor::UseForeground())
		.Padding(FMargin(5.f, 0.f, 5.f, 0.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			[
				SAssignNew(BreakPointCheckBox, SCheckBox)
				//.IsChecked(CodeLine->HasBreakPoint ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
		.IsChecked(this, &SCodeLineItem::BreakPointIsChecked)
		.OnCheckStateChanged(this, &SCodeLineItem::OnClickBreakPoint)
		.CheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
		.CheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
		.CheckedPressedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
		.UncheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Null"))
		.UncheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"))
			]
		]
	+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBox)
			[
				SNew(STextBlock)
				.Text(FText::FromString(FString::FormatAsNumber(CodeLine->Line)))
		.ColorAndOpacity(FSlateColor(FLinearColor(FColor(75, 185, 245, 225))))
		.Font(FScriptEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("TextEditor.NormalText").Font)
			]
		]
		]
		];
}

ECheckBoxState SCodeLineItem::BreakPointIsChecked()const
{
	if (!(FScriptEditor::Get().IsValid()) || !SScriptDebugger::Get())return ECheckBoxState::Unchecked;

	CodeLine->HasBreakPoint = FScriptEditor::Get()->HasBreakPoint(CodeLine->FilePath, CodeLine->Line);

	if (SScriptDebugger::Get()->IsEnterDebugMode && CodeLine->IsHit())
	{
		if (CodeLine->HasBreakPoint)
		{
			BreakPointCheckBox->SetCheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Hit"));
			BreakPointCheckBox->SetCheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Remove"));
		}
		else
		{
			BreakPointCheckBox->SetUncheckedImage(FScriptEditorStyle::Get().GetBrush("Callsatck.Next"));
			BreakPointCheckBox->SetCheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"));
		}
	}
	else
	{
		if (CodeLine->HasBreakPoint)
		{
			BreakPointCheckBox->SetCheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"));
			BreakPointCheckBox->SetCheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Remove"));
		}
		else
		{
			BreakPointCheckBox->SetUncheckedImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.Null"));
			BreakPointCheckBox->SetCheckedHoveredImage(FScriptEditorStyle::Get().GetBrush("Breakpoint.On"));
		}
	}


	return CodeLine->HasBreakPoint ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SCodeLineItem::OnClickBreakPoint(const ECheckBoxState NewCheckedState)
{
	if (NewCheckedState == ECheckBoxState::Checked)
	{
		CodeLine->HasBreakPoint = true;
	}
	else
	{
		CodeLine->HasBreakPoint = false;
	}

	FScriptEditor::Get()->ToggleBreakPoint(CodeLine->FilePath, CodeLine->Line);

	if (SScriptDebugger::Get())
	{
		SScriptDebugger::Get()->SaveDebuggerConfig();
	}
}

void SCodeLineItem::OnBreakConditionCommit(const FText& ConditionText, ETextCommit::Type CommitType)
{
	TSharedPtr<FScriptBreakPointNode> BreakPointNode = FScriptEditor::Get()->GetViewBreakPoint(CodeLine->FilePath, CodeLine->Line);
	if (BreakPointNode.IsValid())
	{
		BreakPointNode->HitCondition = ConditionText;
	}
}

