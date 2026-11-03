#include "DMHUD.h"

#include "CanvasItem.h"
#include "Components/CapsuleComponent.h"
#include "DMBellHorse.h"
#include "DMButler.h"
#include "DMCottonBall.h"
#include "DMCharacter.h"
#include "DMGameState.h"
#include "DMGrabComponent.h"
#include "DMHidingSpot.h"
#include "DMValuable.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/PostProcessVolume.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "DreamMoversHUD"

namespace
{
	const FLinearColor Ink(0.12f, 0.1f, 0.1f);
	const FLinearColor Paper(1.f, 0.97f, 0.9f);
	const FLinearColor Warning(1.f, 0.45f, 0.35f);
	const FLinearColor Good(0.55f, 1.f, 0.6f);
}

void ADMHUD::BeginPlay()
{
	Super::BeginPlay();
	// Canvas text needs a UFont; the legacy engine fonts have no Hangul, the Slate default font does.
	HudFont = NewObject<UFont>(this, TEXT("HudFont"));
	HudFont->FontCacheType = EFontCacheType::Runtime;
	HudFont->GetMutableInternalCompositeFont() = *FCoreStyle::GetDefaultFont();

	BindDreamMaterials();
}

void ADMHUD::BindDreamMaterials()
{
	static const FName CollapseParam(TEXT("Collapse"));
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It)
	{
		for (FWeightedBlendable& Blendable : It->Settings.WeightedBlendables.Array)
		{
			UMaterialInterface* Material = Cast<UMaterialInterface>(Blendable.Object);
			float Unused;
			if (!Material || !Material->GetScalarParameterValue(CollapseParam, Unused))
			{
				continue;
			}
			// The volume is level data shared by every local player; fine for one player per machine.
			UMaterialInstanceDynamic* Dynamic = Cast<UMaterialInstanceDynamic>(Material);
			if (!Dynamic)
			{
				Dynamic = UMaterialInstanceDynamic::Create(Material, this);
				Blendable.Object = Dynamic;
			}
			DreamMaterials.Add(Dynamic);
		}
	}
}

void ADMHUD::UpdateDreamCollapse(float DeltaSeconds)
{
	const ADMGameState* GS = GetWorld()->GetGameState<ADMGameState>();
	if (!GS || DreamMaterials.IsEmpty())
	{
		return;
	}

	float Target = 0.f;
	switch (GS->GetRoundState())
	{
	case EDMRoundState::Playing:
		if (GS->GetRoundDuration() > 0.f)
		{
			// Full color until stability drops to CollapseStartsAt, then fade toward the sketch.
			Target = FMath::GetMappedRangeValueClamped(FVector2f(CollapseStartsAt, 0.f), FVector2f(0.f, CollapseAtWake), GS->GetStability());
		}
		break;
	case EDMRoundState::Failed:
		Target = 1.f; // the dream woke up: only the sketch is left
		break;
	case EDMRoundState::Success:
		Target = 0.f;
		break;
	}

	Collapse = FMath::FInterpTo(Collapse, Target, DeltaSeconds, 1.5f);
	for (UMaterialInstanceDynamic* Dynamic : DreamMaterials)
	{
		Dynamic->SetScalarParameterValue(TEXT("Collapse"), Collapse);
	}
}

void ADMHUD::DrawHUD()
{
	Super::DrawHUD();
	UpdateDreamCollapse(GetWorld()->GetDeltaSeconds());
	if (!Canvas || !HudFont)
	{
		return;
	}
	DrawPriceTags();
	DrawMonsterCues();
	DrawCrosshairAndTarget();
	DrawDownedMessage();
	DrawHidingPrompt();
	DrawRoundInfo();
}

void ADMHUD::DrawCenteredText(const FText& Text, FVector2D Position, const FLinearColor& Color, int32 Size)
{
	FCanvasTextItem Item(Position, Text, FSlateFontInfo(HudFont, Size, TEXT("Bold")), Color);
	Item.bCentreX = true;
	Item.bCentreY = true;
	Item.EnableShadow(Ink);
	Canvas->DrawItem(Item);
}

void ADMHUD::DrawRoundInfo()
{
	const ADMGameState* GS = GetWorld()->GetGameState<ADMGameState>();
	if (!GS)
	{
		return;
	}

	const int32 Seconds = FMath::CeilToInt(GS->GetRemainingSeconds());
	const FText Score = FText::Format(LOCTEXT("Score", "모은 금액 {0} / 목표 {1}"), FText::AsNumber(GS->GetCollectedValue()), FText::AsNumber(GS->GetTargetValue()));
	const FText Timer = FText::Format(LOCTEXT("Timer", "꿈 안정도 {0}%  ·  깨기까지 {1}:{2}"), FText::AsNumber(FMath::CeilToInt(GS->GetStability() * 100.f)),
		FText::AsNumber(Seconds / 60), FText::FromString(FString::Printf(TEXT("%02d"), Seconds % 60)));

	const float CenterX = Canvas->ClipX * 0.5f;
	DrawCenteredText(Score, FVector2D(CenterX, 40.f), GS->IsTargetReached() ? Good : Paper, 22);
	DrawCenteredText(Timer, FVector2D(CenterX, 72.f), Seconds <= 60 ? Warning : Paper, 18);
	if (GS->GetBrokenValue() > 0)
	{
		DrawCenteredText(FText::Format(LOCTEXT("Broken", "깨뜨린 금액 {0}"), FText::AsNumber(GS->GetBrokenValue())), FVector2D(CenterX, 100.f), Warning, 14);
	}

	if (GS->GetRoundState() != EDMRoundState::Playing)
	{
		const bool bWon = GS->GetRoundState() == EDMRoundState::Success;
		DrawCenteredText(bWon ? LOCTEXT("Success", "수거 성공!") : LOCTEXT("Failed", "꿈이 깨버렸다..."),
			FVector2D(CenterX, Canvas->ClipY * 0.4f), bWon ? Good : Warning, 48);
		DrawCenteredText(LOCTEXT("Restarting", "잠시 후 다시 시작합니다"), FVector2D(CenterX, Canvas->ClipY * 0.4f + 50.f), Paper, 18);
	}
}

void ADMHUD::DrawPriceTags()
{
	const APawn* Pawn = GetOwningPawn();
	if (!Pawn)
	{
		return;
	}
	const FVector ViewLocation = Pawn->GetPawnViewLocation();

	for (TActorIterator<ADMValuable> It(GetWorld()); It; ++It)
	{
		const ADMValuable* Valuable = *It;
		const FVector TagLocation = Valuable->GetActorLocation() + FVector(0.f, 0.f, Valuable->GetComponentsBoundingBox().GetExtent().Z + 20.f);
		if (FVector::DistSquared(ViewLocation, TagLocation) > FMath::Square(TagDistance))
		{
			continue;
		}
		const FVector Screen = Project(TagLocation, true);
		if (Screen.Z <= 0.f)
		{
			continue; // behind the camera
		}
		if (Valuable->IsA<ADMCottonBall>())
		{
			DrawCenteredText(Valuable->GetDisplayName(), FVector2D(Screen.X, Screen.Y), Good, 14);
			continue;
		}
		const FLinearColor Color = Valuable->GetCracks() > 0 ? Warning : Paper;
		DrawCenteredText(FText::AsNumber(Valuable->GetCurrentValue()), FVector2D(Screen.X, Screen.Y), Color, 14);
	}
}

void ADMHUD::DrawCrosshairAndTarget()
{
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	DrawCenteredText(FText::FromString(TEXT("·")), Center, Paper, 20);

	const ADMCharacter* Doll = Cast<ADMCharacter>(GetOwningPawn());
	if (!Doll)
	{
		return;
	}
	const UDMGrabComponent* Grab = Doll->GetGrab();
	const ADMValuable* Valuable = Grab->GetHeldValuable();
	if (!Valuable)
	{
		Valuable = Grab->TraceForValuable();
	}
	if (!Valuable)
	{
		return;
	}

	if (Valuable->IsA<ADMCottonBall>())
	{
		DrawCenteredText(FText::Format(LOCTEXT("CottonInfo", "{0}  ·  탈출 지점에 놓으면 부활"), Valuable->GetDisplayName()),
			Center + FVector2D(0.f, 40.f), Good, 16);
		return;
	}
	const FText Info = FText::Format(LOCTEXT("TargetInfo", "{0}  ·  {1}  ·  금 {2}/{3}"),
		Valuable->GetDisplayName(), FText::AsNumber(Valuable->GetCurrentValue()),
		FText::AsNumber(Valuable->GetCracks()), FText::AsNumber(Valuable->GetMaxCracks()));
	DrawCenteredText(Info, Center + FVector2D(0.f, 40.f), Paper, 16);

	// Tell players up front that one of them cannot lift it alone.
	const int32 HoldersNeeded = FMath::CeilToInt(Valuable->GetMassKg() / Grab->GetMaxLiftKg());
	if (HoldersNeeded > 1)
	{
		DrawCenteredText(FText::Format(LOCTEXT("Heavy", "무거움: {0}명 필요 (지금 {1}명)"), FText::AsNumber(HoldersNeeded), FText::AsNumber(Valuable->GetNumHolders())),
			Center + FVector2D(0.f, 64.f), Warning, 14);
	}
}

void ADMHUD::DrawMonsterCues()
{
	const AActor* Viewer = PlayerOwner ? PlayerOwner->GetViewTarget() : nullptr;
	if (!Viewer)
	{
		return;
	}
	const FVector ViewLocation = Viewer->GetActorLocation();
	float Jingle = 0.f;
	bool bCharging = false;

	for (TActorIterator<ADMMonster> It(GetWorld()); It; ++It)
	{
		const ADMMonster* Monster = *It;
		const EDMMonsterState State = Monster->GetMonsterState();

		if (const ADMButler* Butler = Cast<ADMButler>(Monster))
		{
			// "?" grows while it is noticing someone, "!" once it has.
			const bool bChasing = State == EDMMonsterState::Chase;
			const float Suspicion = Butler->GetSuspicion();
			if (!bChasing && Suspicion < 0.05f)
			{
				continue;
			}
			const FVector Head = Monster->GetActorLocation() + FVector(0.f, 0.f, Monster->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 50.f);
			const FVector Screen = Project(Head, true);
			if (Screen.Z > 0.f)
			{
				DrawCenteredText(FText::FromString(bChasing ? TEXT("!") : TEXT("?")), FVector2D(Screen.X, Screen.Y),
					bChasing ? Warning : FMath::Lerp(Paper, Warning, Suspicion), bChasing ? 44 : FMath::RoundToInt(22.f + 18.f * Suspicion));
			}
		}
		else if (Monster->IsA<ADMBellHorse>() && State != EDMMonsterState::Idle)
		{
			const float Distance = FVector::Dist(ViewLocation, Monster->GetActorLocation());
			Jingle = FMath::Max(Jingle, 1.f - Distance / JingleDistance);
			bCharging |= State == EDMMonsterState::Charge && Distance < JingleDistance;
		}
	}

	if (Jingle > 0.f)
	{
		FLinearColor Color = bCharging ? Warning : Paper;
		Color.A = FMath::Clamp(0.25f + Jingle, 0.f, 1.f);
		DrawCenteredText(bCharging ? LOCTEXT("JingleCharge", "딸랑딸랑딸랑! 종소리가 달려온다") : LOCTEXT("Jingle", "딸랑… 딸랑…"),
			FVector2D(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.82f), Color, FMath::RoundToInt(14.f + 10.f * Jingle));
	}
}

void ADMHUD::DrawDownedMessage()
{
	if (GetOwningPawn() || !PlayerOwner || !Cast<ADMCottonBall>(PlayerOwner->GetViewTarget()))
	{
		return;
	}
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.3f);
	DrawCenteredText(LOCTEXT("Downed", "솜이 터져버렸다!"), Center, Warning, 36);
	DrawCenteredText(LOCTEXT("DownedHint", "동료가 솜뭉치를 탈출 지점에 놓아주면 다시 일어납니다"), Center + FVector2D(0.f, 44.f), Paper, 16);
}

void ADMHUD::DrawHidingPrompt()
{
	const ADMCharacter* Doll = Cast<ADMCharacter>(GetOwningPawn());
	if (!Doll)
	{
		return;
	}
	const FVector2D Position(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.62f);
	if (Doll->IsHiding())
	{
		DrawCenteredText(LOCTEXT("Hiding", "서랍장 안에 숨어 있다  ·  E: 나오기"), Position, Paper, 18);
		return;
	}
	const ADMHidingSpot* Spot = ADMHidingSpot::FindReachable(Doll);
	if (!Spot)
	{
		return;
	}
	if (Doll->GetGrab()->GetHeldValuable())
	{
		DrawCenteredText(LOCTEXT("HideHolding", "물건을 놓아야 숨을 수 있다"), Position, Warning, 16);
	}
	else if (!Spot->HasRoom())
	{
		DrawCenteredText(LOCTEXT("HideFull", "서랍장이 꽉 찼다"), Position, Warning, 16);
	}
	else
	{
		DrawCenteredText(FText::Format(LOCTEXT("Hide", "E: 서랍장에 숨기 ({0}/{1})"), FText::AsNumber(Spot->GetNumOccupants()), FText::AsNumber(Spot->GetCapacity())), Position, Paper, 18);
	}
}

#undef LOCTEXT_NAMESPACE
