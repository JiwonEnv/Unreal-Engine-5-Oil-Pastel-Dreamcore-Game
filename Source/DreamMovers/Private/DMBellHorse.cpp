#include "DMBellHorse.h"

#include "Components/CapsuleComponent.h"
#include "DMNoiseSubsystem.h"

#define LOCTEXT_NAMESPACE "DreamMoversBellHorse"

namespace
{
	const FLinearColor Patina(0.22f, 0.42f, 0.36f);
	const FLinearColor Bronze(0.45f, 0.3f, 0.16f);
	const FLinearColor BellBrass(0.85f, 0.62f, 0.26f);
}

ADMBellHorse::ADMBellHorse()
{
	DisplayName = LOCTEXT("Name", "종 달린 말");
	GetCapsuleComponent()->InitCapsuleSize(50.f, 80.f);

	// Facing +X. Thin legs, a long body hung with bells, drooping neck, eyeless head.
	AddPart(TEXT("Body"), CubeShape, FVector(0.f, 0.f, 30.f), FVector(1.7f, 0.55f, 0.55f), Patina, 0.45f, 1.f);
	AddPart(TEXT("Neck"), CylinderShape, FVector(95.f, 0.f, 30.f), FVector(0.25f, 0.25f, 0.8f), Patina, 0.45f, 1.f, FRotator(-50.f, 0.f, 0.f));
	AddPart(TEXT("Head"), CubeShape, FVector(130.f, 0.f, -5.f), FVector(0.5f, 0.25f, 0.3f), Bronze, 0.4f, 1.f, FRotator(-35.f, 0.f, 0.f));
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector At((i < 2 ? 65.f : -65.f), (i % 2 ? 20.f : -20.f), -35.f);
		AddPart(*FString::Printf(TEXT("Leg%d"), i), CylinderShape, At, FVector(0.07f, 0.07f, 0.9f), Bronze, 0.5f, 1.f);
	}
	// Bells of different sizes along the belly and back: polished, so they glint.
	const float BellX[] = { -70.f, -35.f, 0.f, 35.f, 70.f, -50.f, 20.f, 55.f };
	for (int32 i = 0; i < UE_ARRAY_COUNT(BellX); ++i)
	{
		const bool bBelly = i < 5;
		const float Size = 0.16f + 0.05f * (i % 3);
		AddPart(*FString::Printf(TEXT("Bell%d"), i), ConeShape,
			FVector(BellX[i], bBelly ? 0.f : (i % 2 ? 30.f : -30.f), bBelly ? -8.f : 50.f),
			FVector(Size), BellBrass, 0.2f, 1.f);
	}
}

void ADMBellHorse::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Post = GetActorLocation();
		if (UDMNoiseSubsystem* Noise = GetWorld()->GetSubsystem<UDMNoiseSubsystem>())
		{
			NoiseHandle = Noise->OnNoise.AddUObject(this, &ADMBellHorse::OnNoise);
		}
	}
}

void ADMBellHorse::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UDMNoiseSubsystem* Noise = GetWorld()->GetSubsystem<UDMNoiseSubsystem>())
	{
		Noise->OnNoise.Remove(NoiseHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void ADMBellHorse::OnNoise(const FVector& Location, float Radius, AActor* Source)
{
	const float Distance = FVector::Dist(Location, GetActorLocation());
	if (Source == this || Distance > Radius || Distance < IgnoreNoiseWithin)
	{
		return;
	}
	NoiseLocation = Location;
	SetMonsterState(EDMMonsterState::Charge);
	MoveToLocation(NoiseLocation, ChargeSpeed);
}

void ADMBellHorse::ServerThink(float DeltaSeconds)
{
	switch (GetMonsterState())
	{
	case EDMMonsterState::Charge:
		TryCatch();
		if (IsMoveDone())
		{
			StopMoving();
			SetMonsterState(EDMMonsterState::Search);
		}
		break;

	case EDMMonsterState::Search:
		// Standing where the sound was; anyone who bumps into it now is caught.
		TryCatch();
		AddActorWorldRotation(FRotator(0.f, 60.f * DeltaSeconds, 0.f));
		if (StateAge() > SearchSeconds)
		{
			SetMonsterState(EDMMonsterState::Return);
			MoveToLocation(Post, ReturnSpeed);
		}
		break;

	case EDMMonsterState::Return:
		TryCatch();
		if (IsMoveDone())
		{
			SetMonsterState(EDMMonsterState::Idle);
		}
		break;

	default:
		break;
	}
}

#undef LOCTEXT_NAMESPACE
