﻿// Copyright (C) 2019 Gonçalo Marques - All Rights Reserved

#include "UINavInputContainer.h"
#include "UINavWidget.h"
#include "SwapKeysWidget.h"
#include "UINavPCComponent.h"
#include "UINavButton.h"
#include "UINavInputBox.h"
#include "UINavComponent.h"
#include "UINavInputComponent.h"
#include "UINavBlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "IImageWrapper.h"
#include "EnhancedInputComponent.h"
#include "UINavMacros.h"

void UUINavInputContainer::NativeConstruct()
{
	ParentWidget = UUINavWidget::GetOuterObject<UUINavWidget>(this);
	UINavPC = Cast<APlayerController>(GetOwningPlayer())->FindComponentByClass<UUINavPCComponent>();

	bIsFocusable = false;

	if (InputRestrictions.Num() == 0) InputRestrictions.Add(EInputRestriction::None);
	else if (InputRestrictions.Num() > 3) InputRestrictions.SetNum(3);
	KeysPerInput = InputRestrictions.Num();

	SetupInputBoxes();

	Super::NativeConstruct();
}

void UUINavInputContainer::OnAddInputBox_Implementation(class UUINavInputBox* NewInputBox)
{
	if (InputBoxesPanel != nullptr)
	{
		InputBoxesPanel->AddChild(NewInputBox);
	}
}

void UUINavInputContainer::OnKeyRebinded_Implementation(FName InputName, FKey OldKey, FKey NewKey)
{
}

void UUINavInputContainer::OnRebindCancelled_Implementation(ERevertRebindReason RevertReason, FKey PressedKey)
{
}

bool UUINavInputContainer::RequestKeySwap(const FInputCollisionData& InputCollisionData, const int CurrentInputIndex, const int CollidingInputIndex) const
{
	if (SwapKeysWidgetClass != nullptr)
	{
		APlayerController* PC = Cast<APlayerController>(UINavPC->GetOwner());
		USwapKeysWidget* SwapKeysWidget = CreateWidget<USwapKeysWidget>(PC, SwapKeysWidgetClass);
		SwapKeysWidget->CollidingInputBox = InputBoxes[CollidingInputIndex];
		SwapKeysWidget->CurrentInputBox = InputBoxes[CurrentInputIndex];
		SwapKeysWidget->InputCollisionData = InputCollisionData;
		UINavPC->GoToBuiltWidget(SwapKeysWidget, false, false, SpawnKeysWidgetZOrder);
		return true;
	}
	return false;
}

void UUINavInputContainer::ResetKeyMappings()
{
	UUINavBlueprintFunctionLibrary::ResetInputSettings(Cast<APlayerController>(UINavPC->GetOwner()));
	for (UUINavInputBox* InputBox : InputBoxes) InputBox->ResetKeyWidgets();
}

void UUINavInputContainer::SetupInputBoxes()
{
	if (InputBox_BP == nullptr) return;

	InputBoxes.Reset();

	NumberOfInputs = 0;
	for (const TPair<UInputMappingContext*, FInputContainerEnhancedActionDataArray>& Context : EnhancedInputs)
	{
		NumberOfInputs += Context.Value.Actions.Num();
	}

	if (NumberOfInputs == 0)
	{
		DISPLAYERROR(TEXT("Input Container has no Enhanced Input data!"));
		return;
	}
	
	CreateInputBoxes();
}

void UUINavInputContainer::CreateInputBoxes()
{
	if (InputBox_BP == nullptr) return;

	APlayerController* PC = Cast<APlayerController>(UINavPC->GetOwner());
	for (int i = 0; i < NumberOfInputs; ++i)
	{
		UUINavInputBox* NewInputBox = CreateWidget<UUINavInputBox>(this, InputBox_BP);
		InputBoxes.Add(NewInputBox);
		NewInputBox->Container = this;
		NewInputBox->KeysPerInput = KeysPerInput;

		int Index = i;
		for (const TPair<UInputMappingContext*, FInputContainerEnhancedActionDataArray>& Context : EnhancedInputs)
		{
			if (Index >= Context.Value.Actions.Num())
			{
				Index -= Context.Value.Actions.Num();
			}
			else
			{
				NewInputBox->InputContext = Context.Key;
				NewInputBox->InputActionData = Context.Value.Actions[Index];
				NewInputBox->EnhancedInputGroups = Context.Value.InputGroups;
				break;
			}
		}
	}

	for (int i = 0; i < NumberOfInputs; ++i)
	{
		UUINavInputBox* const InputBox = InputBoxes[i];
		InputBox->CreateKeyWidgets();
		OnAddInputBox(InputBox);
	}
}

ERevertRebindReason UUINavInputContainer::CanRegisterKey(UUINavInputBox * InputBox, const FKey NewKey, const int Index, int& OutCollidingActionIndex, int& OutCollidingKeyIndex)
{
	if (!NewKey.IsValid()) return ERevertRebindReason::BlacklistedKey;
	if (KeyWhitelist.Num() > 0 && !KeyWhitelist.Contains(NewKey)) return ERevertRebindReason::NonWhitelistedKey;
	if (KeyBlacklist.Contains(NewKey)) return ERevertRebindReason::BlacklistedKey;
	if (!RespectsRestriction(NewKey, Index)) return ERevertRebindReason::RestrictionMismatch;
	if (InputBox->ContainsKey(NewKey) != INDEX_NONE) return ERevertRebindReason::UsedBySameInput;
	if (!CanUseKey(InputBox, NewKey, OutCollidingActionIndex, OutCollidingKeyIndex)) return ERevertRebindReason::UsedBySameInputGroup;

	return ERevertRebindReason::None;
}

bool UUINavInputContainer::CanUseKey(UUINavInputBox* InputBox, const FKey CompareKey, int& OutCollidingActionIndex, int& OutCollidingKeyIndex) const
{
	if (InputBox->EnhancedInputGroups.Num() == 0) InputBox->EnhancedInputGroups.Add(-1);

	for (int i = 0; i < InputBoxes.Num(); ++i)
	{
		if (InputBox == InputBoxes[i]) continue;

		const int KeyIndex = InputBoxes[i]->ContainsKey(CompareKey);
		if (KeyIndex != INDEX_NONE)
		{
			if (InputBox->EnhancedInputGroups.Contains(-1) ||
				InputBoxes[i]->EnhancedInputGroups.Contains(-1))
			{
				OutCollidingActionIndex = i;
				OutCollidingKeyIndex = KeyIndex;
				return false;
			}

			for (int InputGroup : InputBox->EnhancedInputGroups)
			{
				if (InputBoxes[i]->EnhancedInputGroups.Contains(InputGroup))
				{
					OutCollidingActionIndex = i;
					OutCollidingKeyIndex = KeyIndex;
					return false;
				}
			}
		}
	}

	return true;
}

bool UUINavInputContainer::RespectsRestriction(const FKey CompareKey, const int Index)
{
	const EInputRestriction Restriction = InputRestrictions[Index];

	return UUINavBlueprintFunctionLibrary::RespectsRestriction(CompareKey, Restriction);
}

void UUINavInputContainer::ResetInputBox(const FName InputName, const EAxisType AxisType)
{
	for (UUINavInputBox* InputBox : InputBoxes)
	{
		if (InputBox->InputName.IsEqual(InputName) &&
			InputBox->AxisType == AxisType)
		{
			InputBox->ResetKeyWidgets();
			break;
		}
	}
}

UUINavInputBox* UUINavInputContainer::GetInputBoxInDirection(UUINavInputBox* InputBox, const EUINavigation Direction)
{
	if (!IsValid(InputBox))
	{
		return nullptr;
	}

	int Index;
	if (!InputBoxes.Find(InputBox, Index))
	{
		return nullptr;
	}

	switch (Direction)
	{
		case EUINavigation::Up:
			--Index;
			break;
		case EUINavigation::Down:
			++Index;
			break;
		default:
			return nullptr;
	}

	return InputBoxes.IsValidIndex(Index) ? InputBoxes[Index] : nullptr;
}

UUINavInputBox* UUINavInputContainer::GetOppositeInputBox(const FInputContainerEnhancedActionData& ActionData)
{
	for (UUINavInputBox* InputBox : InputBoxes)
	{
		if (InputBox->InputActionData.Action == ActionData.Action &&
			InputBox->InputActionData.Axis == ActionData.Axis &&
			((InputBox->InputActionData.AxisScale == EAxisType::Positive && ActionData.AxisScale == EAxisType::Negative) ||
			(InputBox->InputActionData.AxisScale == EAxisType::Negative && ActionData.AxisScale == EAxisType::Positive)))
		{
			return InputBox;
		}
	}

	return nullptr;
}

UUINavInputBox* UUINavInputContainer::GetOppositeInputBox(const FName& InputName, const EAxisType AxisType)
{
	for (UUINavInputBox* InputBox : InputBoxes)
	{
		if (InputBox->InputName == InputName &&
			((InputBox->AxisType == EAxisType::Positive && AxisType == EAxisType::Negative) ||
			(InputBox->AxisType == EAxisType::Negative && AxisType == EAxisType::Positive)))
		{
			return InputBox;
		}
	}

	return nullptr;
}

void UUINavInputContainer::GetAxisPropertiesFromMapping(const FEnhancedActionKeyMapping& ActionMapping, bool& bOutPositive, EInputAxis& OutAxis) const
{
	TArray<UInputModifier*> Modifiers(ActionMapping.Modifiers);
	Modifiers.Append(ActionMapping.Action->Modifiers);
	bOutPositive = true;
	if (!UINavPC->IsAxis2D(ActionMapping.Key))
	{
		OutAxis = EInputAxis::X;
	}
	
	for (const UInputModifier* Modifier : Modifiers)
	{
		const UInputModifierNegate* Negate = Cast<UInputModifierNegate>(Modifier);
		if (Negate != nullptr)
		{
			bOutPositive = !bOutPositive;
			continue;
		}

		// TODO: Add support for Scalar input modifier
		
		const UInputModifierSwizzleAxis* Swizzle = Cast<UInputModifierSwizzleAxis>(Modifier);
		if (Swizzle != nullptr && !UINavPC->IsAxis2D(ActionMapping.Key))
		{
			switch(Swizzle->Order)
			{
			case EInputAxisSwizzle::YXZ:
			case EInputAxisSwizzle::YZX:
				OutAxis = EInputAxis::Y;
				break;
			case EInputAxisSwizzle::ZXY:
			case EInputAxisSwizzle::ZYX:
				OutAxis = EInputAxis::Z;
				break;
			}
			continue;
		}
	}
}

int UUINavInputContainer::GetOffsetFromTargetColumn(const bool bTop) const
{
	switch (KeysPerInput)
	{
		case 2:
			if (bTop) return (TargetColumn == ETargetColumn::Right);
			else return -(TargetColumn != ETargetColumn::Right);
			break;
		case 3:
			if (bTop) return static_cast<int>(TargetColumn);
			else return (-2 - static_cast<int>(TargetColumn));
			break;
	}
	return 0;
}

void UUINavInputContainer::GetInputRebindData(const int InputIndex, FInputRebindData& RebindData) const
{
	if (InputBoxes.IsValidIndex(InputIndex))
	{
		RebindData = InputBoxes[InputIndex]->InputData;
	}
}

void UUINavInputContainer::GetEnhancedInputRebindData(const int InputIndex, FInputRebindData& RebindData) const
{
	if (InputBoxes.IsValidIndex(InputIndex))
	{
		RebindData.InputText = InputBoxes[InputIndex]->InputText->GetText();
		RebindData.InputGroups = InputBoxes[InputIndex]->EnhancedInputGroups;
	}
}
