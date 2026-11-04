#include "DMValuable.h"

#include "Components/StaticMeshComponent.h"
#include "DMGameState.h"
#include "DMNoiseSubsystem.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Settling into place at BeginPlay produces small hits that should not count.
	constexpr double HitGraceSeconds = 1.0;
	// One collision usually reports several contacts over a few frames.
	constexpr double CrackCooldownSeconds = 0.25;
	constexpr double NoiseCooldownSeconds = 0.3;
	constexpr float LoudBumpSpeed = 800.f;
	// Angular damping while held keeps an off-center grab from spinning the item like a propeller.
	constexpr float HeldAngularDamping = 8.f;
	constexpr float FreeAngularDamping = 0.1f;
}

ADMValuable::ADMValuable()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	SetPhysicsReplicationMode(EPhysicsReplicationMode::PredictiveInterpolation);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetNotifyRigidBodyCollision(true);
	// The extraction zone finds items by overlap.
	Mesh->SetGenerateOverlapEvents(true);
	// Items get carried around; monsters path through the empty room and push them aside.
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetAngularDamping(FreeAngularDamping);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Mesh->SetStaticMesh(CubeMesh.Object);
	// Explicit, because the engine shapes default to the grid material, which has no Color parameter.
	Mesh->SetMaterial(0, ShapeMaterial.Object);

	DisplayName = NSLOCTEXT("DreamMovers", "DefaultValuableName", "Antique");
}

void ADMValuable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// No tint here: a dynamic material made in the editor would be saved into the map.
	Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);
}

void ADMValuable::BeginPlay()
{
	Super::BeginPlay();
	ApplyMassAndColor();

	if (HasAuthority())
	{
		CurrentValue = BaseValue;
		HitsEnabledTime = GetWorld()->GetTimeSeconds() + HitGraceSeconds;
		Mesh->OnComponentHit.AddDynamic(this, &ADMValuable::OnMeshHit);
	}
	else
	{
		// Clients never run hit logic.
		SetActorTickEnabled(false);
	}
}

void ADMValuable::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	LastVelocity = Mesh->GetPhysicsLinearVelocity();
}

void ADMValuable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADMValuable, CurrentValue);
	DOREPLIFETIME(ADMValuable, Cracks);
	DOREPLIFETIME(ADMValuable, NumHolders);
	DOREPLIFETIME_CONDITION(ADMValuable, Spec, COND_InitialOnly);
}

void ADMValuable::InitFromSpec(const FDMValuableSpec& InSpec)
{
	Spec = InSpec;
	ApplySpec();
}

void ADMValuable::OnRep_Spec()
{
	ApplySpec();
}

void ADMValuable::ApplySpec()
{
	if (!Spec.Mesh)
	{
		return;
	}
	DisplayName = Spec.Name;
	BaseValue = Spec.Value;
	MassKg = Spec.MassKg;
	CrackSpeed = Spec.CrackSpeed;
	Tint = Spec.Tint;
	Roughness = Spec.Roughness;
	Metallic = Spec.Metallic;
	Mesh->SetStaticMesh(Spec.Mesh);
	if (Spec.Material)
	{
		Mesh->SetMaterial(0, Spec.Material);
	}
	SetActorScale3D(Spec.Size / 100.f);
	// The tint instance belongs to the old material.
	TintMaterial = nullptr;
	ApplyMassAndColor();
}

void ADMValuable::AddHolder()
{
	++NumHolders;
	Mesh->SetAngularDamping(HeldAngularDamping);
	Mesh->WakeRigidBody();
}

void ADMValuable::RemoveHolder()
{
	NumHolders = FMath::Max(0, NumHolders - 1);
	if (NumHolders == 0)
	{
		Mesh->SetAngularDamping(FreeAngularDamping);
	}
}

void ADMValuable::Extract()
{
	if (!HasAuthority() || bExtracted)
	{
		return;
	}
	bExtracted = true;
	if (ADMGameState* GS = GetWorld()->GetGameState<ADMGameState>())
	{
		GS->AddCollected(CurrentValue);
	}
	Destroy();
}

void ADMValuable::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < HitsEnabledTime || Now - LastCrackTime < CrackCooldownSeconds)
	{
		return;
	}

	FVector OtherVelocity = FVector::ZeroVector;
	if (OtherComp && OtherComp->IsSimulatingPhysics())
	{
		OtherVelocity = OtherComp->GetPhysicsLinearVelocity();
	}
	const float ImpactSpeed = FMath::Abs(FVector::DotProduct(LastVelocity - OtherVelocity, Hit.ImpactNormal));
	const bool bCracks = ImpactSpeed >= CrackSpeed;
	if (ImpactSpeed >= QuietSpeed && (bCracks || Now - LastNoiseTime >= NoiseCooldownSeconds))
	{
		LastNoiseTime = Now;
		const float Radius = FMath::GetMappedRangeValueClamped(FVector2D(QuietSpeed, LoudBumpSpeed), BumpNoiseRadius, ImpactSpeed);
		UDMNoiseSubsystem::Report(this, Hit.ImpactPoint, bCracks ? Radius * 1.5f : Radius, this);
	}
	if (bCracks)
	{
		LastCrackTime = Now;
		Crack(ImpactSpeed);
	}
}

void ADMValuable::Crack(float ImpactSpeed)
{
	++Cracks;
	if (Cracks >= MaxCracks)
	{
		UDMNoiseSubsystem::Report(this, GetActorLocation(), BreakNoiseRadius, this);
		CurrentValue = 0;
		if (ADMGameState* GS = GetWorld()->GetGameState<ADMGameState>())
		{
			GS->AddBroken(BaseValue);
		}
		Destroy();
		return;
	}

	// A hit twice as hard as CrackSpeed costs twice as much.
	const float Loss = LossPerCrack * (ImpactSpeed / CrackSpeed);
	CurrentValue = FMath::Max(0, CurrentValue - FMath::RoundToInt(BaseValue * Loss));
	OnRep_Cracks();
}

void ADMValuable::OnRep_Cracks()
{
	ApplyMassAndColor();
}

void ADMValuable::ApplyMassAndColor()
{
	Mesh->SetMassOverrideInKg(NAME_None, MassKg, true);

	if (!TintMaterial)
	{
		TintMaterial = Mesh->CreateDynamicMaterialInstance(0);
	}
	if (TintMaterial)
	{
		// Each crack darkens it, so damage reads at a glance before the HUD number does.
		const float Damage = MaxCracks > 0 ? float(Cracks) / float(MaxCracks) : 0.f;
		TintMaterial->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(Tint, FLinearColor(0.08f, 0.06f, 0.06f), Damage * 0.8f));
		// A cracked surface loses its shine. Ignored by materials without these parameters.
		TintMaterial->SetScalarParameterValue(TEXT("Roughness"), FMath::Lerp(Roughness, 0.8f, Damage));
		TintMaterial->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	}
}
