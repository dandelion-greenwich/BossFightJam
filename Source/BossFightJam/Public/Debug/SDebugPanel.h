#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class SVerticalBox;

/**
 * The contents of the Debug.Panel window.
 *
 * Slate rather than UMG because this lives in a top-level OS window, outside
 * the game viewport entirely - and because a debug tool that never ships is
 * not worth an asset to maintain.
 *
 * Nothing here caches game state. Every readout is a lambda that Slate
 * re-evaluates each frame, so the panel cannot go stale and needs no tick,
 * timer or refresh button.
 */
class BOSSFIGHTJAM_API SDebugPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDebugPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/** A titled block with a hairline above it. */
	TSharedRef<SWidget> MakeSection(const FString& Title, TSharedRef<SWidget> Content) const;

	/** Label on the left, live value on the right. */
	TSharedRef<SWidget> MakeReadout(const FString& Label, TFunction<FString()> ValueGetter) const;

	/** Small button running Action. */
	TSharedRef<SWidget> MakeButton(const FString& Label, TFunction<void()> Action) const;

	TSharedRef<SWidget> BuildEncounterSection();
	TSharedRef<SWidget> BuildPlayerSection();
	TSharedRef<SWidget> BuildBossSection();
	TSharedRef<SWidget> BuildHackSection();

	/** Rebuilt when the hack count changes, since rows are per-hack. */
	TSharedPtr<SVerticalBox> HackRows;
	int32 CachedHackCount = -1;

	void RefreshHackRows();
	virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;
};

#endif
