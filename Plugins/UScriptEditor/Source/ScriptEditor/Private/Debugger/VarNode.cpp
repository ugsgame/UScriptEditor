// Fill out your copyright notice in the Description page of Project Settings.

#include "VarNode.h"
#include "VarWatcher.h"
TSet<UObject*> UVarNode::HasUpdateState;
void FVarNode::GetChild(TArray<TSharedRef<FVarNode>> &OutChildren)
{
	if (UNode)
	{
		for (auto& Child : UNode->Children)
		{
			OutChildren.Add(Child->FNode.ToSharedRef());
		}
		for (auto& Child : UNode->KeyChildren)
		{
			OutChildren.Add(Child->FNode.ToSharedRef());
		}
	}
}

FText FVarNode::GetName()
{
	if (UNode)
		return UNode->Name;
	else
		return FText::FromString("nil");
}

FText FVarNode::GetValue()
{
	if (UNode)
		return UNode->Value;
	else
		return FText::FromString("nil");
}



void UVarNode::AddChild(UVarNode* Child, bool bIsKey /*= false*/)
{
	Child->Parent = this;
	if (bIsKey)
	{
		KeyChildren.Add(Child);
		KeyChildren.StableSort([&](const UVarNode& a, const UVarNode& b) {
			FString a_KeyString = a.Name.ToString();
			FString b_KeyString = b.Name.ToString();
			if (a_KeyString.IsNumeric() != b_KeyString.IsNumeric())
			{
				return b_KeyString.IsNumeric();
			}
			else if (a_KeyString.IsNumeric())
			{
				return FCString::Atoi(*a_KeyString) < FCString::Atoi(*b_KeyString);
			}
			else
				return a.Name.CompareTo(b.Name)<0; });
	}
	else
	{
		Children.Add(Child);
		Children.StableSort([&](const UVarNode& a, const UVarNode& b) {
			FString a_KeyString = a.Name.ToString();
			FString b_KeyString = b.Name.ToString();
			if (a_KeyString.IsNumeric() != b_KeyString.IsNumeric())
			{
				return b_KeyString.IsNumeric();
			}
			else if (a_KeyString.IsNumeric())
			{
				return FCString::Atoi(*a_KeyString) < FCString::Atoi(*b_KeyString);
			}
			else
				return a.Name.CompareTo(b.Name) < 0; });
	}
	Refresh();
}

void UVarNode::Release()
{
	FNode->UNode = nullptr;
	if (Parent)
	{
		Parent->RemoveChild(this);
	}
	else
		RemoveFromRoot();
}

bool UVarNode::ConsumeNeedExpand()
{
	if (FNode->bNeedExpandNextUpdate && !HasExpand)
	{
		HasExpand = true;
		FNode->bNeedExpandNextUpdate = false;
		return true;
	}
	return false;
}

void UVarNode::AddToRootArr()
{
	SVarWatcher::Get()->VarTreeRoot.Add(FNode.ToSharedRef());
	Refresh();
}

bool UVarNode::HasChild()
{
	return Children.Num() > 0 || KeyChildren.Num() > 0;
}

void UVarNode::RemoveFromRoot()
{
	SVarWatcher::Get()->VarTreeRoot.Remove(FNode.ToSharedRef());
	Refresh();
}

void UVarNode::RemoveChild(UVarNode* Child)
{
	Children.Remove(Child);
	KeyChildren.Remove(Child);
	Refresh();
}

void UVarNode::Refresh()
{
	if (SVarWatcher::Get()->VarTreePtr.IsValid())
	{
		SVarWatcher::Get()->VarTreePtr.Pin()->RequestTreeRefresh();
	}
}

bool UVarNode::bIsExpanding()
{
	return FNode->bNeedExpandNextUpdate;
}

bool UVarNode::NeedUpdate(UObject* GameInstance)
{
	if (SVarWatcher::Get()->bNeedUpdateData)
	{
		HasUpdateState.Reset();
		SVarWatcher::Get()->bNeedUpdateData = false;
	}

	if (!HasUpdateState.Contains(GameInstance))
	{
		HasUpdateState.Add(GameInstance);
		return true;
	}
	else
		return false;
}

bool UVarNode::NeedShowFunction()
{
	return SVarWatcher::Get()->bShowFunction;
}



