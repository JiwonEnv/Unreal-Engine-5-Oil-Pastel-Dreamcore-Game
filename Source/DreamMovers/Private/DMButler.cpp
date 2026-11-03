#include "DMButler.h"

#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DMCharacter.h"
#include "DMNoiseSubsystem.h"
#include "DreamMovers.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

#define LOCTEXT_NAMESPACE "DreamMoversButler"

namespace
{
	const FLinearColor Velvet(0.42f, 0.04f, 0.06f);
	const FLinearColor Brass(0.85f, 0.62f, 0.28f);
	const FLinearColor Wax(0.95f, 0.92f, 0.82f);
	const FLinearColor Shadow(0.07f, 0.06f, 0.06f);
	const FLinearColor CalmLight(1.f, 0.72f, 0.42f);
	const FLinearColor AlarmLight(1.f, 0.3f, 0.15f);
}

ADMButler::ADMButler()
{
	DisplayName = LOCTEXT("Name", "촛대 집사");
	// About 190 cm: a head taller than the dolls' whole body.
	GetCapsuleComponent()->InitCapsuleSize(30.f, 95.f);

	const FVector Thin(0.07f, 0.07f, 0.95f);
	AddPart(TEXT("LegL"), CylinderShape, FVector(0.f, -12.f, -47.f), Thin, Shadow);
	AddPart(TEXT("LegR"), CylinderShape, FVector(0.f, 12.f, -47.f), Thin, Shadow);
	AddPart(TEXT("Coat"), CylinderShape, FVector(0.f, 0.f, 25.f), FVector(0.42f, 0.36f, 0.7f), Velvet, 0.85f);
	AddPart(TEXT("WaxL"), CylinderShape, FVector(10.f, -20.f, 5.f), FVector(0.12f, 0.12f, 0.35f), Wax, 0.4f);
	AddPart(TEXT("WaxR"), CylinderShape, FVector(10.f, 20.f, 5.f), FVector(0.12f, 0.12f, 0.35f), Wax, 0.4f);
	AddPart(TEXT("ArmL"), CylinderShape, FVector(12.f, -30.f, 25.f), FVector(0.05f, 0.05f, 0.6f), Shadow, 0.6f, 0.f, FRotator(0.f, 0.f, -20.f));
	AddPart(TEXT("ArmR"), CylinderShape, FVector(12.f, 30.f, 25.f), FVector(0.05f, 0.05f, 0.6f), Shadow, 0.6f, 0.f, FRotator(0.f, 0.f, 20.f));
	AddPart(TEXT("BellL"), ConeShape, FVector(18.f, -40.f, -8.f), FVector(0.14f), Brass, 0.2f, 1.f);
	AddPart(TEXT("BellR"), ConeShape, FVector(18.f, 40.f, -8.f), FVector(0.14f), Brass, 0.2f, 1.f);
	AddPart(TEXT("Dish"), CylinderShape, FVector(0.f, 0.f, 64.f), FVector(0.7f, 0.7f, 0.03f), Brass, 0.2f, 1.f);
	AddPart(TEXT("Cup"), CylinderShape, FVector(0.f, 0.f, 78.f), FVector(0.18f, 0.18f, 0.25f), Brass, 0.2f, 1.f);
	AddPart(TEXT("Candle"), CylinderShape, FVector(0.f, 0.f, 100.f), FVector(0.1f, 0.1f, 0.25f), Wax, 0.5f);
	AddPart(TEXT("Flame"), SphereShape, FVector(0.f, 0.f, 118.f), FVector(0.07f, 0.07f, 0.13f), FLinearColor(1.f, 0.65f, 0.2f), 0.9f);

	Candlelight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Candlelight"));
	Candlelight->SetupAttachment(GetCapsuleComponent());
	Candlelight->SetRelativeLocationAndRotation(FVector(12.f, 0.f, 112.f), FRotator(-14.f, 0.f, 0.f));
	Candlelight->SetIntensityUnits(ELightUnits::Candelas);
	Candlelight->SetIntensity(150.f);
	Candlelight->SetLightColor(CalmLight);
	Candlelight->SetCastShadows(true);

	PatrolPoints = { FVector::ZeroVector };
}

void ADMButler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADMButler, Suspicion);
}

void ADMButler::BeginPlay()
{
	// The light is the sight: keep them in sync with the tuning values.
	Candlelight->SetAttenuationRadius(SightRange);
	Candlelight->SetOuterConeAngle(SightHalfAngle);
	Candlelight->SetInnerConeAngle(SightHalfAngle * 0.6f);

	Super::BeginPlay();

	if (HasAuthority())
	{
		for (const FVector& Point : PatrolPoints)
		{
			WorldPatrol.Add(GetActorTransform().TransformPosition(Point));
		}
		ResumePatrol();
	}
}

void ADMButler::OnMonsterStateChanged()
{
	const bool bAlarmed = GetMonsterState() == EDMMonsterState::Chase;
	Candlelight->SetLightColor(bAlarmed ? AlarmLight : CalmLight);
	Candlelight->SetIntensity(bAlarmed ? 300.f : 150.f);
}

bool ADMButler::CanSee(const ADMCharacter* Doll, float* OutDistance) const
{
	if (!Doll || Doll->IsDowned() || Doll->IsHiding())
	{
		return false; // a closed wardrobe blocks the candlelight
	}
	const FVector Eye = Candlelight->GetComponentLocation();
	const FVector TargetPoint = Doll->GetPawnViewLocation();
	const FVector ToTarget = TargetPoint - Eye;
	const float Distance = ToTarget.Size();
	if (Distance > SightRange)
	{
		return false;
	}
	const float Cos = FVector::DotProduct(ToTarget / FMath::Max(Distance, 1.f), Candlelight->GetForwardVector());
	if (Cos < FMath::Cos(FMath::DegreesToRadians(SightHalfAngle)))
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DMButlerSight), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, TargetPoint, ECC_Visibility, Params) && Hit.GetActor() != Doll)
	{
		return false; // behind a wall, furniture or a carried antique
	}
	if (OutDistance)
	{
		*OutDistance = Distance;
	}
	return true;
}

ADMCharacter* ADMButler::FindVisibleDoll(float& OutDistance) const
{
	ADMCharacter* Best = nullptr;
	OutDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ADMCharacter> It(GetWorld()); It; ++It)
	{
		float Distance;
		if (CanSee(*It, &Distance) && Distance < OutDistance)
		{
			Best = *It;
			OutDistance = Distance;
		}
	}
	return Best;
}

void ADMButler::RingBell()
{
	LastBellTime = GetWorld()->GetTimeSeconds();
	UDMNoiseSubsystem::Report(this, GetActorLocation(), BellRadius, this);
}

void ADMButler::ResumePatrol()
{
	SpotProgress = 0.f;
	Target.Reset();
	SetMonsterState(EDMMonsterState::Patrol);
	if (WorldPatrol.IsEmpty())
	{
		return;
	}
	// Continue from the nearest point rather than walking back across the house.
	float Best = TNumericLimits<float>::Max();
	for (int32 i = 0; i < WorldPatrol.Num(); ++i)
	{
		const float Dist = FVector::DistSquared(WorldPatrol[i], GetActorLocation());
		if (Dist < Best)
		{
			Best = Dist;
			PatrolIndex = i;
		}
	}
	PauseUntil = 0.0;
	MoveToLocation(WorldPatrol[PatrolIndex], PatrolSpeed);
}

void ADMButler::ServerThink(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (TryCatch())
	{
		ResumePatrol();
		return;
	}

	switch (GetMonsterState())
	{
	case EDMMonsterState::Patrol:
	case EDMMonsterState::Search:
	{
		float Distance;
		if (ADMCharacter* Seen = FindVisibleDoll(Distance))
		{
			// Up close it notices about twice as fast.
			SpotProgress += DeltaSeconds * (2.f - Distance / SightRange);
			if (SpotProgress >= SpotSeconds)
			{
				Target = Seen;
				LastKnownLocation = Seen->GetActorLocation();
				LastSeenTime = Now;
				SetMonsterState(EDMMonsterState::Chase);
				RingBell();
				MoveToActor(Seen, ChaseSpeed);
				break;
			}
		}
		else
		{
			SpotProgress = FMath::Max(0.f, SpotProgress - DeltaSeconds * 0.5f);
		}

		if (GetMonsterState() == EDMMonsterState::Search)
		{
			if (IsMoveDone() && StateAge() > SearchSeconds)
			{
				ResumePatrol();
			}
		}
		else if (WorldPatrol.Num() > 1 && IsMoveDone())
		{
			if (PauseUntil == 0.0)
			{
				PauseUntil = Now + PausePerPoint;
			}
			else if (Now >= PauseUntil)
			{
				PauseUntil = 0.0;
				PatrolIndex = (PatrolIndex + 1) % WorldPatrol.Num();
				UE_LOG(LogDreamMovers, Verbose, TEXT("%s: at %s, heading to patrol point %d"), *GetName(), *GetActorLocation().ToCompactString(), PatrolIndex);
				MoveToLocation(WorldPatrol[PatrolIndex], PatrolSpeed);
			}
		}
		break;
	}

	case EDMMonsterState::Chase:
	{
		ADMCharacter* Chased = Target.Get();
		if (!Chased || Chased->IsDowned())
		{
			ResumePatrol();
			break;
		}
		if (CanSee(Chased))
		{
			LastSeenTime = Now;
			LastKnownLocation = Chased->GetActorLocation();
		}
		if (Now - LastSeenTime > LoseSightSeconds)
		{
			// Lost them: check where they were last seen, then give up.
			SetMonsterState(EDMMonsterState::Search);
			SpotProgress = 0.f;
			MoveToLocation(LastKnownLocation, ChaseSpeed);
			break;
		}
		if (Now - LastRepathTime > 0.4)
		{
			LastRepathTime = Now;
			MoveToActor(Chased, ChaseSpeed);
		}
		if (Now - LastBellTime > BellInterval)
		{
			RingBell();
		}
		break;
	}

	default:
		ResumePatrol();
		break;
	}

	Suspicion = GetMonsterState() == EDMMonsterState::Chase ? 255 : uint8(FMath::Clamp(SpotProgress / SpotSeconds, 0.f, 1.f) * 255.f);
}

#undef LOCTEXT_NAMESPACE
