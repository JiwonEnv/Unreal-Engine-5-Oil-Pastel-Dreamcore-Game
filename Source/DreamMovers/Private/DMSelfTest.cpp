#include "DMSelfTest.h"

#include "Components/StaticMeshComponent.h"
#include "DMBellHorse.h"
#include "DMButler.h"
#include "DMCharacter.h"
#include "DMCottonBall.h"
#include "DMExtractionZone.h"
#include "DMGrabComponent.h"
#include "DMNoiseSubsystem.h"
#include "DMValuable.h"
#include "DreamMovers.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/App.h"
#include "UnrealClient.h"

namespace
{
	// Seconds after spawn at which each step runs.
	const double StepTimes[] = { 3.0, 3.5, 5.5, 6.0, 6.5, 7.0, 9.0, 10.0, 11.5, 18.5, 19.5, 22.5, 24.5, 25.5, 27.5, 29.5 };
}

ADMSelfTest::ADMSelfTest()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADMSelfTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StepTime += DeltaSeconds;
	while (NextStep < int32(UE_ARRAY_COUNT(StepTimes)) && StepTime >= StepTimes[NextStep])
	{
		Step(NextStep++);
	}
}

ADMCharacter* ADMSelfTest::Doll() const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	return PC ? Cast<ADMCharacter>(PC->GetPawn()) : nullptr;
}

AActor* ADMSelfTest::FindValuable(const TCHAR* NamePrefix) const
{
	for (TActorIterator<ADMValuable> It(GetWorld()); It; ++It)
	{
		if (It->GetDisplayName().ToString().StartsWith(NamePrefix))
		{
			return *It;
		}
	}
	return nullptr;
}

bool ADMSelfTest::PlaceDollFacing(const FVector& Location, const FVector& LookAt)
{
	ADMCharacter* D = Doll();
	if (!D)
	{
		return false;
	}
	const FRotator Facing = (LookAt - (Location + FVector(0.f, 0.f, D->BaseEyeHeight))).Rotation();
	const bool bMoved = D->TeleportTo(Location, FRotator(0.f, Facing.Yaw, 0.f));
	if (AController* C = D->GetController())
	{
		C->SetControlRotation(Facing);
	}
	return bMoved;
}

void ADMSelfTest::LookLevel()
{
	if (const ADMCharacter* D = Doll())
	{
		if (AController* C = D->GetController())
		{
			C->SetControlRotation(FRotator(0.f, C->GetControlRotation().Yaw, 0.f));
		}
	}
}

void ADMSelfTest::Check(bool bPassed, const FString& What)
{
	(bPassed ? Passed : Failed)++;
	UE_LOG(LogDreamMovers, Display, TEXT("[SelfTest] %s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *What);
}

void ADMSelfTest::Shot(const TCHAR* Name)
{
	if (FApp::CanEverRender())
	{
		FScreenshotRequest::RequestScreenshot(FString::Printf(TEXT("SelfTest_%s"), Name), true, false);
	}
}

void ADMSelfTest::Step(int32 Index)
{
	ADMCharacter* D = Doll();
	ADMButler* Butler = nullptr;
	ADMBellHorse* Horse = nullptr;
	for (TActorIterator<ADMButler> It(GetWorld()); It; ++It) { Butler = *It; }
	for (TActorIterator<ADMBellHorse> It(GetWorld()); It; ++It) { Horse = *It; }

	switch (Index)
	{
	case 0: // Light item: the porcelain clown just inside the parlor.
		Check(D != nullptr, TEXT("listen server has a doll"));
		Check(Butler && Horse, TEXT("both monsters are in the map"));
		LightItem = FindValuable(TEXT("도자기 광대"));
		if (LightItem.IsValid() && D)
		{
			LightItemStartZ = LightItem->GetActorLocation().Z;
			const FVector At = LightItem->GetActorLocation();
			PlaceDollFacing(FVector(At.X - 150.f, At.Y, 60.f), At);
		}
		break;
	case 1:
		if (D)
		{
			D->GetGrab()->TryGrab();
			LookLevel();
		}
		break;
	case 2:
		Check(D && D->GetGrab()->GetHeldValuable() == LightItem.Get(), TEXT("thread grabbed the 5 kg item"));
		Check(LightItem.IsValid() && LightItem->GetActorLocation().Z > LightItemStartZ + 25.f,
			FString::Printf(TEXT("5 kg item lifted (z %.0f -> %.0f)"), LightItemStartZ, LightItem.IsValid() ? LightItem->GetActorLocation().Z : 0.f));
		Shot(TEXT("1_Holding"));
		break;
	case 3:
		if (D) { D->GetGrab()->Release(); }
		break;
	case 4: // Heavy item: one doll must not be able to lift the carousel horse.
		HeavyItem = FindValuable(TEXT("회전목마"));
		if (HeavyItem.IsValid() && D)
		{
			HeavyItemStartZ = HeavyItem->GetActorLocation().Z;
			const FVector At = HeavyItem->GetActorLocation();
			PlaceDollFacing(FVector(At.X - 170.f, At.Y, 60.f), At);
		}
		break;
	case 5:
		// A step later: aiming reads the camera, which updates once a frame.
		if (D)
		{
			D->GetGrab()->TryGrab();
			LookLevel();
		}
		break;
	case 6:
		Check(D && D->GetGrab()->GetHeldValuable() == HeavyItem.Get(), TEXT("thread grabbed the 32 kg item"));
		Check(HeavyItem.IsValid() && HeavyItem->GetActorLocation().Z < HeavyItemStartZ + 25.f,
			FString::Printf(TEXT("32 kg item NOT lifted by one doll (z %.0f -> %.0f)"), HeavyItemStartZ, HeavyItem.IsValid() ? HeavyItem->GetActorLocation().Z : 0.f));
		if (D) { D->GetGrab()->Release(); }
		break;
	case 7: // Out of the way, back in the hall.
		PlaceDollFacing(FVector(-1000.f, 0.f, 60.f), FVector(0.f, 0.f, 90.f));
		break;
	case 8: // A noise in the back room: the blind horse should charge to it.
		UDMNoiseSubsystem::Report(this, FVector(1350.f, 450.f, 50.f), 2000.f, nullptr);
		break;
	case 9:
		Check(Horse && Horse->GetMonsterState() != EDMMonsterState::Idle,
			FString::Printf(TEXT("horse reacted to the noise (state %s)"), Horse ? *UEnum::GetValueAsString(Horse->GetMonsterState()) : TEXT("none")));
		// Stand in front of the butler's candle, facing it.
		if (Butler && D)
		{
			const FVector Front = Butler->GetActorLocation() + Butler->GetActorForwardVector() * 300.f;
			if (!PlaceDollFacing(FVector(Front.X, Front.Y, 60.f), Butler->GetActorLocation() + FVector(0.f, 0.f, 80.f)))
			{
				UE_LOG(LogDreamMovers, Warning, TEXT("[SelfTest] could not place the doll in front of the butler"));
			}
		}
		break;
	case 10:
		Check(Butler && (Butler->GetMonsterState() == EDMMonsterState::Chase || Butler->GetSuspicion() > 0.f || !Doll()),
			FString::Printf(TEXT("butler noticed the doll (state %s, suspicion %.2f)"),
				Butler ? *UEnum::GetValueAsString(Butler->GetMonsterState()) : TEXT("none"), Butler ? Butler->GetSuspicion() : 0.f));
		Shot(TEXT("2_Butler"));
		break;
	case 11:
	{
		const APlayerController* PC = GetWorld()->GetFirstPlayerController();
		const AActor* View = PC ? PC->GetViewTarget() : nullptr;
		Check(Cast<ADMCottonBall>(View) != nullptr, FString::Printf(TEXT("downed player views their cotton ball (view target %s)"), View ? *View->GetName() : TEXT("none")));
		break;
	}
	case 12:
	{
		TActorIterator<ADMCottonBall> Ball(GetWorld());
		Check(Doll() == nullptr && Ball, TEXT("butler caught the doll and a cotton ball spawned"));
		Shot(TEXT("3_Downed"));
		// Carry it out the quick way.
		TActorIterator<ADMExtractionZone> Zone(GetWorld());
		if (Ball && Zone)
		{
			Ball->GetMesh()->SetWorldLocation(Zone->GetActorLocation(), false, nullptr, ETeleportType::TeleportPhysics);
		}
		break;
	}
	case 13:
		Check(Doll() != nullptr, TEXT("cotton ball in the extraction zone revived the doll"));
		break;
	case 14:
		Shot(TEXT("4_Revived"));
		break;
	case 15:
		UE_LOG(LogDreamMovers, Display, TEXT("[SelfTest] DONE: %d passed, %d failed"), Passed, Failed);
		FPlatformMisc::RequestExit(false, TEXT("DreamMovers self test finished"));
		break;
	default:
		break;
	}
}
