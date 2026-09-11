#include "Debug/SDebugPanel.h"

#if !UE_BUILD_SHIPPING

#include "Debug/DebugControlLibrary.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "DebugPanel"

namespace
{
	/** Shown wherever a value has no game to read from. */
	const FString NoValue = TEXT("--");

	FString PhaseName()
	{
		return UDebugControlLibrary::IsGameRunning() && UDebugControlLibrary::HasBoss()
			? FString::Printf(TEXT("%d"), static_cast<int32>(UDebugControlLibrary::GetBossPhase()) + 1)
			: NoValue;
	}
}

void SDebugPanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(8.f)
		[
			SNew(SScrollBox)

			+ SScrollBox::Slot()
			[
				BuildEncounterSection()
			]

			+ SScrollBox::Slot()
			[
				BuildPlayerSection()
			]

			+ SScrollBox::Slot()
			[
				BuildBossSection()
			]

			+ SScrollBox::Slot()
			[
				BuildHackSection()
			]

			+ SScrollBox::Slot()
			[
				BuildPoolSection()
			]
		]
	];

	RefreshHackRows();
}

// ---------------------------------------------------------------- Helpers

TSharedRef<SWidget> SDebugPanel::MakeSection(const FString& Title, TSharedRef<SWidget> Content) const
{
	return SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 10.f, 0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Title))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.82f, 0.88f)))
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SSeparator)
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			Content
		];
}

TSharedRef<SWidget> SDebugPanel::MakeReadout(const FString& Label, TFunction<FString()> ValueGetter) const
{
	return SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.FillWidth(0.45f)
		[
			SNew(STextBlock).Text(FText::FromString(Label))
		]

		+ SHorizontalBox::Slot()
		.FillWidth(0.55f)
		[
			// Slate re-evaluates this every frame, which is what makes the
			// readout live without any tick of our own.
			SNew(STextBlock)
			.Text_Lambda([ValueGetter]() { return FText::FromString(ValueGetter()); })
		];
}

TSharedRef<SWidget> SDebugPanel::MakeButton(const FString& Label, TFunction<void()> Action) const
{
	return SNew(SButton)
		.Text(FText::FromString(Label))
		.ContentPadding(FMargin(6.f, 2.f))
		.IsEnabled_Lambda([]() { return UDebugControlLibrary::IsGameRunning(); })
		.OnClicked_Lambda([Action]()
		{
			Action();
			return FReply::Handled();
		});
}

// ---------------------------------------------------------------- Sections

TSharedRef<SWidget> SDebugPanel::BuildEncounterSection()
{
	return MakeSection(TEXT("ENCOUNTER"),
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("State"), []()
			{
				return UDebugControlLibrary::IsGameRunning()
					? UEnum::GetDisplayValueAsText(UDebugControlLibrary::GetEncounterState()).ToString()
					: NoValue;
			})
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("Restart"), []() { UDebugControlLibrary::RestartEncounter(); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("Start Encounter"), []() { UDebugControlLibrary::StartEncounter(); })
			]
		]);
}

TSharedRef<SWidget> SDebugPanel::BuildPlayerSection()
{
	return MakeSection(TEXT("PLAYER"),
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Health"), []()
			{
				return UDebugControlLibrary::IsGameRunning()
					? FString::Printf(TEXT("%.0f / %.0f"),
						UDebugControlLibrary::GetPlayerHealth(), UDebugControlLibrary::GetPlayerMaxHealth())
					: NoValue;
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Ammo"), []()
			{
				return UDebugControlLibrary::IsGameRunning()
					? FString::Printf(TEXT("%d / %d"),
						UDebugControlLibrary::GetPlayerAmmo(), UDebugControlLibrary::GetPlayerMagazineSize())
					: NoValue;
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Protection"), []()
			{
				if (!UDebugControlLibrary::IsGameRunning())
				{
					return NoValue;
				}

				// Two independent shields, so both are worth showing separately.
				const bool bGod = UDebugControlLibrary::IsPlayerInvulnerable();
				const bool bIFrames = UDebugControlLibrary::IsPlayerInHitImmunity();

				if (bGod && bIFrames) { return FString(TEXT("god + i-frames")); }
				if (bGod)             { return FString(TEXT("god mode")); }
				if (bIFrames)         { return FString(TEXT("i-frames")); }
				return FString(TEXT("none"));
			})
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("Damage", "Damage"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("10"), []() { UDebugControlLibrary::DamagePlayer(10.f); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("25"), []() { UDebugControlLibrary::DamagePlayer(25.f); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("50"), []() { UDebugControlLibrary::DamagePlayer(50.f); })
			]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("Kill"), []() { UDebugControlLibrary::KillPlayer(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("Heal Full"), []() { UDebugControlLibrary::HealPlayer(0.f); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("Refill Ammo"), []() { UDebugControlLibrary::RefillAmmo(); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("Toggle God"), []()
				{
					UDebugControlLibrary::SetPlayerGodMode(!UDebugControlLibrary::IsPlayerInvulnerable());
				})
			]
		]);
}

TSharedRef<SWidget> SDebugPanel::BuildBossSection()
{
	return MakeSection(TEXT("BOSS"),
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Health"), []()
			{
				return UDebugControlLibrary::HasBoss()
					? FString::Printf(TEXT("%.0f / %.0f"),
						UDebugControlLibrary::GetBossHealth(), UDebugControlLibrary::GetBossMaxHealth())
					: FString(TEXT("no boss registered"));
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Phase"), []() { return PhaseName(); })
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Shield"), []()
			{
				if (!UDebugControlLibrary::HasBoss())
				{
					return NoValue;
				}

				FString Line = UEnum::GetDisplayValueAsText(UDebugControlLibrary::GetBossShieldState()).ToString();

				const float Remaining = UDebugControlLibrary::GetBossShieldRemaining();
				if (Remaining > 0.f)
				{
					Line += FString::Printf(TEXT("  %.1fs left"), Remaining);
				}

				return Line;
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Status"), []()
			{
				if (!UDebugControlLibrary::HasBoss())
				{
					return NoValue;
				}

				if (UDebugControlLibrary::IsBossStunned())    { return FString(TEXT("stunned")); }
				if (UDebugControlLibrary::IsBossTransitioning()) { return FString(TEXT("transitioning")); }
				if (!UDebugControlLibrary::IsBossAttacking())  { return FString(TEXT("idle")); }
				return FString(TEXT("fighting"));
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Attack"), []()
			{
				if (!UDebugControlLibrary::HasBoss())
				{
					return NoValue;
				}

				const int32 Step = UDebugControlLibrary::GetBossStepNumber();
				const int32 Length = UDebugControlLibrary::GetBossSequenceLength();

				if (Step == 0)
				{
					// A phase with no steps authored is the usual reason the
					// boss stands there doing nothing, so say so plainly.
					return Length == 0 ? FString(TEXT("no sequence authored")) : FString(TEXT("idle"));
				}

				return FString::Printf(TEXT("%d/%d  %s"), Step, Length,
					*UDebugControlLibrary::GetBossCurrentAttackName());
			})
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("BossDamage", "Damage"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("10"), []() { UDebugControlLibrary::DamageBoss(10.f); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("25"), []() { UDebugControlLibrary::DamageBoss(25.f); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("50"), []() { UDebugControlLibrary::DamageBoss(50.f); })
			]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(STextBlock).Text(LOCTEXT("SetPhase", "Phase"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("1"), []() { UDebugControlLibrary::SetBossPhase(EBossPhase::Phase1); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("2"), []() { UDebugControlLibrary::SetBossPhase(EBossPhase::Phase2); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("3"), []() { UDebugControlLibrary::SetBossPhase(EBossPhase::Phase3); })
			]
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("Kill"), []() { UDebugControlLibrary::KillBoss(); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
			[
				MakeButton(TEXT("Drop Shield 10s"), []() { UDebugControlLibrary::DropBossShield(10.f); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("Stun 5s"), []() { UDebugControlLibrary::StunBoss(5.f); })
			]
		]);
}

TSharedRef<SWidget> SDebugPanel::BuildHackSection()
{
	return MakeSection(TEXT("HACKS"),
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(HackRows, SVerticalBox)
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeButton(TEXT("Reset All Cooldowns"), []() { UDebugControlLibrary::ResetHackCooldowns(); })
			]
		]);
}

TSharedRef<SWidget> SDebugPanel::BuildPoolSection()
{
	return MakeSection(TEXT("PROJECTILE POOL"),
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("In flight"), []()
			{
				return UDebugControlLibrary::IsGameRunning()
					? FString::Printf(TEXT("%d"), UDebugControlLibrary::GetPoolActiveCount())
					: NoValue;
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Free"), []()
			{
				return UDebugControlLibrary::IsGameRunning()
					? FString::Printf(TEXT("%d"), UDebugControlLibrary::GetPoolFreeCount())
					: NoValue;
			})
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeReadout(TEXT("Total spawned"), []()
			{
				if (!UDebugControlLibrary::IsGameRunning())
				{
					return NoValue;
				}

				// Climbing mid-fight means the pool ran dry and had to grow,
				// which is the one number here worth watching.
				return FString::Printf(TEXT("%d"), UDebugControlLibrary::GetPoolTotalCount());
			})
		]);
}

// ---------------------------------------------------------------- Hack rows

void SDebugPanel::RefreshHackRows()
{
	if (!HackRows.IsValid())
	{
		return;
	}

	HackRows->ClearChildren();

	const int32 Count = UDebugControlLibrary::GetHackCount();
	CachedHackCount = Count;

	if (Count == 0)
	{
		HackRows->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NoHacks", "No hack component found."))
		];
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		HackRows->AddSlot().AutoHeight()
		[
			MakeReadout(FString::Printf(TEXT("%d"), i + 1), [i]()
			{
				const FHackDebugInfo Info = UDebugControlLibrary::GetHackInfo(i);

				FString Line = Info.DisplayName.ToString() + TEXT("  ") + Info.State;
				if (Info.RemainingSeconds > 0.f)
				{
					Line += FString::Printf(TEXT("  %.1fs"), Info.RemainingSeconds);
				}
				return Line;
			})
		];
	}
}

void SDebugPanel::Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, CurrentTime, DeltaTime);

	// Values update themselves through their lambdas; only the number of rows
	// needs rebuilding, and only when it actually changes - which happens when
	// PIE starts or stops.
	const int32 Count = UDebugControlLibrary::GetHackCount();
	if (Count != CachedHackCount)
	{
		RefreshHackRows();
	}
}

#undef LOCTEXT_NAMESPACE

#endif
