#include "DMGrabComponent.h"

#include "Components/StaticMeshComponent.h"
#include "DMValuable.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr float Gravity = 980.f;
	// Thread thickness as a scale of the 100 cm engine cylinder.
	constexpr float ThreadThickness = 0.015f;
}

UDMGrabComponent::UDMGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Forces added here are consumed by this frame's physics step.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

void UDMGrabComponent::BeginPlay()
{
	Super::BeginPlay();

	// Purely cosmetic and rebuilt on every machine from the replicated grab state.
	ThreadMesh = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("GrabThread"));
	ThreadMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	ThreadMesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	ThreadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ThreadMesh->SetCastShadow(false);
	ThreadMesh->SetUsingAbsoluteLocation(true);
	ThreadMesh->SetUsingAbsoluteRotation(true);
	ThreadMesh->SetUsingAbsoluteScale(true);
	ThreadMesh->SetupAttachment(GetOwner()->GetRootComponent());
	ThreadMesh->SetVisibility(false);
	ThreadMesh->RegisterComponent();

	ThreadMaterial = ThreadMesh->CreateDynamicMaterialInstance(0);
	if (ThreadMaterial)
	{
		ThreadMaterial->SetVectorParameterValue(TEXT("Color"), ThreadColor);
	}
}

void UDMGrabComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner()->HasAuthority())
	{
		ServerDropHeld();
	}
	Super::EndPlay(EndPlayReason);
}

void UDMGrabComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDMGrabComponent, HeldValuable);
	DOREPLIFETIME(UDMGrabComponent, LocalGrabOffset);
}

void UDMGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority())
	{
		// The item may have been banked or shattered by someone else.
		if (HeldValuable && (!IsValid(HeldValuable) || HeldValuable->IsExtracted()))
		{
			HeldValuable = nullptr;
		}
		if (HeldValuable)
		{
			ServerApplyForce(DeltaTime);
		}
	}
	UpdateThreadVisual();
}

FVector UDMGrabComponent::GetViewLocation() const
{
	const APawn* Pawn = CastChecked<APawn>(GetOwner());
	return Pawn->GetPawnViewLocation();
}

FVector UDMGrabComponent::GetAimDirection() const
{
	// Server: the controller's rotation, kept current by character movement updates.
	// Other clients: actor yaw plus the replicated view pitch.
	const APawn* Pawn = CastChecked<APawn>(GetOwner());
	return Pawn->GetBaseAimRotation().Vector();
}

FVector UDMGrabComponent::GetThreadStart() const
{
	const APawn* Pawn = CastChecked<APawn>(GetOwner());
	const FRotationMatrix Aim(Pawn->GetBaseAimRotation());
	// From the lower right of the view, like a hand casting the thread.
	return GetViewLocation() + Aim.GetUnitAxis(EAxis::X) * 30.f + Aim.GetUnitAxis(EAxis::Y) * 18.f - Aim.GetUnitAxis(EAxis::Z) * 18.f;
}

ADMValuable* UDMGrabComponent::TraceForValuable(FVector* OutHitPoint) const
{
	const FVector Start = GetViewLocation();
	const FVector End = Start + GetAimDirection() * GrabRange;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DMGrabTrace), false, GetOwner());

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return nullptr;
	}
	ADMValuable* Valuable = Cast<ADMValuable>(Hit.GetActor());
	if (Valuable && OutHitPoint)
	{
		*OutHitPoint = Hit.ImpactPoint;
	}
	return Valuable;
}

void UDMGrabComponent::TryGrab()
{
	FVector HitPoint;
	if (ADMValuable* Target = TraceForValuable(&HitPoint))
	{
		HoldDistance = FMath::Clamp(FVector::Dist(GetViewLocation(), HitPoint), MinHoldDistance, MaxHoldDistance);
		ServerGrab(Target, HitPoint);
	}
}

void UDMGrabComponent::Release()
{
	ServerRelease();
}

void UDMGrabComponent::AdjustHoldDistance(float Direction)
{
	if (!HeldValuable || FMath::IsNearlyZero(Direction))
	{
		return;
	}
	HoldDistance = FMath::Clamp(HoldDistance + FMath::Sign(Direction) * HoldDistanceStep, MinHoldDistance, MaxHoldDistance);
	ServerSetHoldDistance(HoldDistance);
}

void UDMGrabComponent::ServerGrab_Implementation(ADMValuable* Target, FVector_NetQuantize WorldGrabPoint)
{
	if (!IsValid(Target) || Target->IsExtracted())
	{
		return;
	}
	// The client traced against a slightly older world; allow some slack but no grabbing across the map.
	const float Distance = FVector::Dist(GetViewLocation(), WorldGrabPoint);
	if (Distance > GrabRange * 1.5f)
	{
		return;
	}

	ServerDropHeld();
	HeldValuable = Target;
	LocalGrabOffset = Target->GetMesh()->GetComponentTransform().InverseTransformPosition(WorldGrabPoint);
	HoldDistance = FMath::Clamp(Distance, MinHoldDistance, MaxHoldDistance);
	Target->AddHolder();
}

void UDMGrabComponent::ServerRelease_Implementation()
{
	ServerDropHeld();
}

void UDMGrabComponent::ServerSetHoldDistance_Implementation(float NewDistance)
{
	HoldDistance = FMath::Clamp(NewDistance, MinHoldDistance, MaxHoldDistance);
}

void UDMGrabComponent::ServerDropHeld()
{
	if (IsValid(HeldValuable))
	{
		HeldValuable->RemoveHolder();
	}
	HeldValuable = nullptr;
}

void UDMGrabComponent::ServerApplyForce(float DeltaTime)
{
	UStaticMeshComponent* Body = HeldValuable->GetMesh();
	const FVector GrabPoint = Body->GetComponentTransform().TransformPosition(LocalGrabOffset);
	const FVector Target = GetViewLocation() + GetAimDirection() * HoldDistance;
	const FVector Offset = Target - GrabPoint;

	if (Offset.Size() > BreakDistance)
	{
		ServerDropHeld();
		return;
	}

	// Mass-scaled damped spring, so light and heavy items feel equally responsive until the cap kicks in.
	// Gravity compensation is shared between holders; otherwise two holders would fling light items upward.
	const float Mass = HeldValuable->GetMassKg();
	const int32 Holders = FMath::Max(1, HeldValuable->GetNumHolders());
	const FVector PointVelocity = Body->GetPhysicsLinearVelocityAtPoint(GrabPoint);
	const FVector Accel = Offset * FMath::Square(SpringFrequency)
		- PointVelocity * (2.f * SpringDampingRatio * SpringFrequency)
		+ FVector(0.f, 0.f, Gravity / Holders);

	// The cap is what makes heavy items a two-person job.
	const FVector Force = (Accel * Mass).GetClampedToMaxSize(MaxLiftKg * Gravity);
	Body->AddForceAtLocation(Force, GrabPoint);
}

void UDMGrabComponent::UpdateThreadVisual()
{
	if (!ThreadMesh)
	{
		return;
	}
	if (!IsValid(HeldValuable))
	{
		ThreadMesh->SetVisibility(false);
		return;
	}

	const FVector Start = GetThreadStart();
	const FVector End = HeldValuable->GetMesh()->GetComponentTransform().TransformPosition(LocalGrabOffset);
	const FVector Span = End - Start;
	const float Length = Span.Size();
	if (Length < KINDA_SMALL_NUMBER)
	{
		ThreadMesh->SetVisibility(false);
		return;
	}

	// The engine cylinder is 100 cm tall along Z and centered on its pivot.
	ThreadMesh->SetWorldLocationAndRotation((Start + End) * 0.5f, FRotationMatrix::MakeFromZ(Span / Length).Rotator());
	ThreadMesh->SetWorldScale3D(FVector(ThreadThickness, ThreadThickness, Length / 100.f));
	ThreadMesh->SetVisibility(true);
}
