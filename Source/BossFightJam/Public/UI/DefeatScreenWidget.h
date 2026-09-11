#pragma once

#include "CoreMinimal.h"
#include "UI/EndScreenWidget.h"
#include "DefeatScreenWidget.generated.h"

/** Base class for WBP_DefeatScreen. All behaviour is in UEndScreenWidget. */
UCLASS(Abstract)
class BOSSFIGHTJAM_API UDefeatScreenWidget : public UEndScreenWidget
{
	GENERATED_BODY()

protected:
	virtual EEncounterState GetTriggerState() const override { return EEncounterState::Defeat; }
};
