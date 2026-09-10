#include "UI/HackKeyWidget.h"

#include "Components/TextBlock.h"

void UHackKeyWidget::SetKey(int32 InKeyNumber, bool bInHighlighted, EHackRowState InRowState)
{
	KeyNumber = InKeyNumber;
	bHighlighted = bInHighlighted;
	RowState = InRowState;

	if (KeyText)
	{
		KeyText->SetText(FText::AsNumber(KeyNumber));
	}

	OnKeyStateChanged(KeyNumber, bHighlighted, RowState);
}
