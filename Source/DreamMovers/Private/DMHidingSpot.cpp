#include "DMHidingSpot.h"

#include "Components/StaticMeshComponent.h"
#include "DMCharacter.h"
#include "DMNoiseSubsystem.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADMHidingSpot::ADMHidingSpot()
{
	bReplicates = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetStaticMesh(CubeMesh.Object);
	Body->SetMaterial(0, ShapeMaterial.Object);
	Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
}

void ADMHidingSpot::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyShape();
}

void ADMHidingSpot::Configure(const FVector& InSize, int32 InCapacity)
{
	Size = InSize;
	Capacity = FMath::Clamp(InCapacity, 1, 2);
	ApplyShape();
}

void ADMHidingSpot::OnRep_Shape()
{
	ApplyShape();
}

void ADMHidingSpot::ApplyShape()
{
	// The engine cube is 100 cm and centered.
	Body->SetRelativeLocation(FVector(0.f, 0.f, Size.Z * 0.5f));
	Body->SetRelativeScale3D(Size / 100.f);
}

void ADMHidingSpot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADMHidingSpot, NumOccupants);
	DOREPLIFETIME_CONDITION(ADMHidingSpot, Size, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ADMHidingSpot, Capacity, COND_InitialOnly);
}

ADMHidingSpot* ADMHidingSpot::FindReachable(const ADMCharacter* Doll)
{
	if (!Doll)
	{
		return nullptr;
	}
	ADMHidingSpot* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<ADMHidingSpot> It(Doll->GetWorld()); It; ++It)
	{
		// Distance to the front face, measured in the wardrobe's own space.
		const FVector Local = It->GetActorTransform().InverseTransformPosition(Doll->GetActorLocation());
		const float Front = Local.X - It->Size.X * 0.5f;
		const bool bInFront = Front > -10.f && Front < It->ReachDistance && FMath::Abs(Local.Y) < It->Size.Y * 0.5f + 30.f;
		if (bInFront && Front < BestDistance)
		{
			Best = *It;
			BestDistance = Front;
		}
	}
	return Best;
}

void ADMHidingSpot::PurgeOccupants()
{
	Occupants.RemoveAll([](const TWeakObjectPtr<ADMCharacter>& Doll) { return !Doll.IsValid() || Doll->IsDowned(); });
	NumOccupants = Occupants.Num();
}

FVector ADMHidingSpot::GetSlotLocation(int32 Slot) const
{
	const float SideOffset = Capacity > 1 ? (Slot == 0 ? -0.25f : 0.25f) * Size.Y : 0.f;
	return GetActorTransform().TransformPosition(FVector(0.f, SideOffset, 60.f));
}

FVector ADMHidingSpot::GetExitLocation(int32 Slot) const
{
	const float SideOffset = Capacity > 1 ? (Slot == 0 ? -0.25f : 0.25f) * Size.Y : 0.f;
	return GetActorTransform().TransformPosition(FVector(Size.X * 0.5f + 50.f, SideOffset, 60.f));
}

bool ADMHidingSpot::TryEnter(ADMCharacter* Doll)
{
	if (!HasAuthority() || !Doll)
	{
		return false;
	}
	PurgeOccupants();
	if (!HasRoom() || Occupants.Contains(Doll))
	{
		return false;
	}
	// Take the free side so two dolls do not end up on top of each other.
	int32 Slot = 0;
	if (Capacity > 1 && Occupants.Num() == 1)
	{
		const FVector Local = GetActorTransform().InverseTransformPosition(Occupants[0]->GetActorLocation());
		Slot = Local.Y < 0.f ? 1 : 0;
	}
	Occupants.Add(Doll);
	NumOccupants = Occupants.Num();
	UDMNoiseSubsystem::Report(this, GetActorLocation(), DoorNoiseRadius, this);
	// Facing out through the doors, the way you would peek through the gap.
	Doll->EnterHiding(this, GetSlotLocation(Slot), GetActorRotation());
	return true;
}

void ADMHidingSpot::Leave(ADMCharacter* Doll)
{
	if (!HasAuthority() || !Doll)
	{
		return;
	}
	const int32 Index = Occupants.IndexOfByKey(Doll);
	if (Index == INDEX_NONE)
	{
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Doll->GetActorLocation());
	const int32 Slot = Capacity > 1 && Local.Y > 0.f ? 1 : 0;
	Occupants.RemoveAt(Index);
	NumOccupants = Occupants.Num();
	UDMNoiseSubsystem::Report(this, GetActorLocation(), DoorNoiseRadius, this);
	Doll->LeaveHiding(GetExitLocation(Slot));
}
