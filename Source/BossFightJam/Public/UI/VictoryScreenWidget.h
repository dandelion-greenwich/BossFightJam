#pragma once

#include "CoreMinimal.h"
#include "UI/EndScreenWidget.h"
#include "VictoryScreenWidget.generated.h"

/** Base class for WBP_VictoryScreen. All behaviour is in UEndScreenWidget. */
UCLASS(Abstract)
class BOSSFIGHTJAM_API UVictoryScreenWidget : public UEndScreenWidget
{
	GENERATED_BODY()

protected:
	virtual EEncounterState GetTriggerState() const override { return EEncounterState::Victory; }
};
