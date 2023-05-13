// Copyright (C) 2019 Gonçalo Marques - All Rights Reserved

#include "UINavComponent.h"
#include "UINavWidget.h"
#include "UINavPCComponent.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Internationalization.h"
#include "UINavMacros.h"
#include "UINavSettings.h"

UUINavComponent::UUINavComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	bIsFocusable = true;

	PaletteCategory = NSLOCTEXT("UMG", "UINav", "UINav");

	ComponentText = FText::FromString(TEXT("Button Text"));
}

void UUINavComponent::NativeConstruct()
{
	NavButton->OnClicked.RemoveAll(this);
	NavButton->OnClicked.AddDynamic(this, &UUINavComponent::OnButtonClicked);
	NavButton->OnPressed.RemoveAll(this);
	NavButton->OnPressed.AddDynamic(this, &UUINavComponent::OnButtonPressed);
	NavButton->OnReleased.RemoveAll(this);
	NavButton->OnReleased.AddDynamic(this, &UUINavComponent::OnButtonReleased);
	NavButton->OnHovered.RemoveAll(this);
	NavButton->OnHovered.AddDynamic(this, &UUINavComponent::OnButtonHovered);
	NavButton->OnUnhovered.RemoveAll(this);
	NavButton->OnUnhovered.AddDynamic(this, &UUINavComponent::OnButtonUnhovered);

	Super::NativeConstruct();

	if (!IsValid(ParentWidget))
	{
		ParentWidget = UUINavWidget::GetOuterObject<UUINavWidget>(this);

		if (!IsValid(ParentWidget))
		{
			DISPLAYERROR("UI Nav Component isn't in a UINavWidget!");
		}
		else if (!IsValid(ParentWidget->GetFirstComponent()))
		{
			ParentWidget->SetFirstComponent(this);
		}
	}
}

bool UUINavComponent::Initialize()
{
	return Super::Initialize();
}

FReply UUINavComponent::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	
	if (!IsValid(ParentWidget))
	{
		return Reply;
	}

	if (FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		if (!ParentWidget->TryConsumeNavigation())
		{
			ParentWidget->StartedSelect();
		}
	}

	const EUINavigation Direction = FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent);
	if (Direction != EUINavigation::Invalid)
	{
		ParentWidget->UINavPC->NotifyNavigationKeyPressed(InKeyEvent.GetKey(), Direction);
		if (Direction == EUINavigation::Next)
		{
			ParentWidget->PropagateOnNext();
		}
		else if (Direction == EUINavigation::Previous)
		{
			ParentWidget->PropagateOnPrevious();
		}
	}

	return Reply;
}

FReply UUINavComponent::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FReply Reply = Super::NativeOnKeyUp(InGeometry, InKeyEvent);

	if (!IsValid(ParentWidget))
	{
		return Reply;
	}

	if (FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Accept)
	{
		if (!ParentWidget->TryConsumeNavigation())
		{
			ParentWidget->StoppedSelect();
		}
	}
	else if (FSlateApplication::Get().GetNavigationActionFromKey(InKeyEvent) == EUINavigationAction::Back)
	{
		if (!ParentWidget->TryConsumeNavigation())
		{
			ParentWidget->StoppedReturn();
		}
	}
	else
	{
		const EUINavigation Direction = FSlateApplication::Get().GetNavigationDirectionFromKey(InKeyEvent);
		if (Direction != EUINavigation::Invalid)
		{
			ParentWidget->UINavPC->NotifyNavigationKeyReleased(InKeyEvent.GetKey(), Direction);
		}
	}

	return Reply;
}

FReply UUINavComponent::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		IsValid(ParentWidget) &&
		IsValid(ParentWidget->UINavPC) &&
		ParentWidget->UINavPC->IsListeningToInputRebind())
	{
		ParentWidget->UINavPC->ProcessRebind(EKeys::LeftMouseButton);
	}

	return Reply;
}

FReply UUINavComponent::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply Reply = Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		IsValid(ParentWidget) &&
		IsValid(ParentWidget->UINavPC) &&
		ParentWidget->UINavPC->IsListeningToInputRebind())
	{
		ParentWidget->UINavPC->ProcessRebind(EKeys::LeftMouseButton);
	}

	return Reply;
}

void UUINavComponent::OnNavigatedTo_Implementation()
{
}

void UUINavComponent::OnNavigatedFrom_Implementation()
{
}

void UUINavComponent::HandleFocusReceived()
{
	if (!IsComponentValid(true) || !IsButtonValid())
	{
		return;
	}

	if (IsValid(ParentWidget))
	{
		ParentWidget->NavigatedTo(this);
	}
}

void UUINavComponent::HandleFocusLost()
{
}

void UUINavComponent::OnButtonClicked()
{
	OnNativeClicked.Broadcast();
	OnClicked.Broadcast();
}

void UUINavComponent::OnButtonPressed()
{
	OnNativePressed.Broadcast();
	OnPressed.Broadcast();

	if (IsValid(ParentWidget))
	{
		ParentWidget->OnPressedComponent(this);
	}
}

void UUINavComponent::OnButtonReleased()
{
	OnNativeReleased.Broadcast();
	OnReleased.Broadcast();

	if (IsValid(ParentWidget))
	{
		ParentWidget->OnReleasedComponent(this);
	}
}

void UUINavComponent::OnButtonHovered()
{
	if (IsValid(ParentWidget))
	{
		ParentWidget->OnHoveredComponent(this);
	}
}

void UUINavComponent::OnButtonUnhovered()
{
	if (IsValid(ParentWidget))
	{
		ParentWidget->OnUnhoveredComponent(this);
	}
}

void UUINavComponent::SetText(const FText& Text)
{
	ComponentText = Text;

	if (IsValid(NavText))
	{
		NavText->SetText(ComponentText);
	}
}

void UUINavComponent::SwitchTextColorTo(FLinearColor Color)
{
	if (IsValid(NavText) && bUseTextColor)
	{
		NavText->SetColorAndOpacity(Color);
	}
}

void UUINavComponent::SwitchTextColorToDefault()
{
	SwitchTextColorTo(TextDefaultColor);
}

void UUINavComponent::SwitchTextColorToNavigated()
{
	SwitchTextColorTo(TextNavigatedColor);
}

bool UUINavComponent::IsComponentValid(const bool bIgnoreDisabledUINavButton) const
{
	return (Visibility != ESlateVisibility::Collapsed &&
			Visibility != ESlateVisibility::Hidden &&
			(!bIgnoreDisabledUINavButton || bIsEnabled));
}

bool UUINavComponent::IsButtonValid(const bool bIgnoreDisabledUINavButton) const
{
	return (NavButton->Visibility != ESlateVisibility::Collapsed &&
			NavButton->Visibility != ESlateVisibility::Hidden &&
			(!bIgnoreDisabledUINavButton || NavButton->bIsEnabled));
}

FReply UUINavComponent::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);

	HandleFocusReceived();

	return Reply;
}

void UUINavComponent::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);

	HandleFocusLost();
}

void UUINavComponent::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);

	const bool bHadFocus = PreviousFocusPath.ContainsWidget(NavButton->TakeWidget());
	const bool bHasFocus = NewWidgetPath.ContainsWidget(NavButton->TakeWidget());

	const FName WidgetType = NewWidgetPath.GetLastWidget()->GetType();
	if (!WidgetType.IsEqual(FName(TEXT("SObjectWidget"))) &&
		!WidgetType.IsEqual(FName(TEXT("SButton"))))
	{
		SetFocus();
		return;
	}

	if (!bHadFocus && bHasFocus)
	{
		HandleFocusReceived();
	}
	else if (bHadFocus && !bHasFocus)
	{
		HandleFocusLost();
	}
}

FNavigationReply UUINavComponent::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply)
{
	FNavigationReply Reply = Super::NativeOnNavigation(MyGeometry, InNavigationEvent, InDefaultReply);

	if (!IsValid(ParentWidget) || !IsValid(ParentWidget->UINavPC))
	{
		return Reply;
	}

	if (!ParentWidget->UINavPC->AllowsNavigatingDirection(InNavigationEvent.GetNavigationType()))
	{
		return FNavigationReply::Stop();
	}

	if (ParentWidget->TryConsumeNavigation())
	{
		return FNavigationReply::Stop();
	}

	if (!ParentWidget->UINavPC->TryNavigateInDirection(InNavigationEvent.GetNavigationType()))
	{
		return FNavigationReply::Stop();
	}

	if (GetDefault<UUINavSettings>()->bStopNextPreviousNavigation)
	{
		if (InNavigationEvent.GetNavigationType() == EUINavigation::Next)
		{
			ParentWidget->OnNext();
			return FNavigationReply::Stop();
		}
		
		if (InNavigationEvent.GetNavigationType() == EUINavigation::Previous)
		{
			ParentWidget->OnPrevious();
			return FNavigationReply::Stop();
		}
	}

	return Reply;
}

void UUINavComponent::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (IsValid(NavText))
	{
		NavText->SetText(ComponentText);

		if (bOverride_Font)
		{
			NavText->SetFont(FontOverride);
		}
		else
		{
			FontOverride = NavText->Font;
		}

		if (bUseTextColor)
		{
			NavText->SetColorAndOpacity(TextDefaultColor);
		}
	}
	
	if (IsValid(NavButton))
	{
		if (bOverride_Style)
		{
			NavButton->SetStyle(StyleOverride);
		}
		else
		{
			StyleOverride = NavButton->WidgetStyle;
		}
	}
}

void UUINavComponent::SwitchButtonStyle(const EButtonStyle NewStyle, const bool bRevertStyle /*= true*/)
{
	if (NewStyle == ForcedStyle)
	{
		return;
	}

	CurrentStyle = GetStyleFromButtonState();
	if (NewStyle == CurrentStyle && ForcedStyle == EButtonStyle::None) return;

	const bool bWasForcePressed = ForcedStyle == EButtonStyle::Pressed;

	if (bRevertStyle)
	{
		RevertButtonStyle();
	}

	SwapStyle(NewStyle, CurrentStyle);

	if (NewStyle == EButtonStyle::Pressed && CurrentStyle != EButtonStyle::Pressed)
	{
		SwapPadding();
	}
	else if (bWasForcePressed)
	{
		SwapPadding();
	}

	if (NewStyle != CurrentStyle)
	{
		ForcedStyle = NewStyle;
	}

	if (NewStyle == EButtonStyle::Hovered && !HasAnyUserFocus())
	{
		USoundBase* HoverSound = Cast<USoundBase>(NavButton->WidgetStyle.HoveredSlateSound.GetResourceObject());
		if (HoverSound != nullptr)
		{
			PlaySound(HoverSound);
		}
	}
}

void UUINavComponent::RevertButtonStyle()
{
	if (ForcedStyle == EButtonStyle::None) return;

	SwapStyle(ForcedStyle, CurrentStyle);

	ForcedStyle = EButtonStyle::None;
}

void UUINavComponent::SwapStyle(EButtonStyle Style1, EButtonStyle Style2)
{
	FButtonStyle Style = NavButton->WidgetStyle;
	FSlateBrush TempState;

	switch (Style1)
	{
	case EButtonStyle::Normal:
		TempState = Style.Normal;
		switch (Style2)
		{
		case EButtonStyle::Hovered:
			Style.Normal = Style.Hovered;
			Style.Hovered = TempState;
			break;
		case EButtonStyle::Pressed:
			Style.Normal = Style.Pressed;
			Style.Pressed = TempState;
			break;
		}
		break;
	case EButtonStyle::Hovered:
		TempState = Style.Hovered;
		switch (Style2)
		{
		case EButtonStyle::Normal:
			Style.Hovered = Style.Normal;
			Style.Normal = TempState;
			break;
		case EButtonStyle::Pressed:
			Style.Hovered = Style.Pressed;
			Style.Pressed = TempState;
			break;
		}
		break;
	case EButtonStyle::Pressed:
		TempState = Style.Pressed;
		switch (Style2)
		{
		case EButtonStyle::Normal:
			Style.Pressed = Style.Normal;
			Style.Normal = TempState;
			break;
		case EButtonStyle::Hovered:
			Style.Pressed = Style.Hovered;
			Style.Hovered = TempState;
			break;
		}
		break;
	}

	NavButton->SetStyle(Style);
}

void UUINavComponent::SwapPadding()
{
	const FButtonStyle Style = NavButton->WidgetStyle;
	UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(NavButton->Slot);
	const FMargin PressedPadding = Style.PressedPadding - Style.NormalPadding;
	if (OverlaySlot != nullptr)
	{
		OverlaySlot->SetPadding(OverlaySlot->Padding == PressedPadding ? FMargin(0.0f) : PressedPadding);
	}
}

EButtonStyle UUINavComponent::GetStyleFromButtonState()
{
	if (NavButton->IsPressed()) return EButtonStyle::Pressed;
	else if (NavButton->IsHovered()) return EButtonStyle::Hovered;
	else return EButtonStyle::Normal;
}
