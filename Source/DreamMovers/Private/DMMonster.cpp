#include "DMMonster.h"

#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DMCharacter.h"
#include "DreamMovers.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ADMMonster::ADMMonster()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 300.f, 0.f);
	Move->bUseRVOAvoidance = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Engine/BasicShapes/Cone.Cone"));
	CubeShape = Cube.Object;
	CylinderShape = Cylinder.Object;
	SphereShape = Sphere.Object;
	ConeShape = Cone.Object;

	// The proto master has Roughness/Metallic so brass parts glint under the oil pastel shader.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProtoShape(TEXT("/Game/Proto/Materials/M_Proto_Shape.M_Proto_Shape"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	PartMaterial = ProtoShape.Succeeded() ? ProtoShape.Object : BasicShape.Object;
}

void ADMMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADMMonster, MonsterState);
}

UStaticMeshComponent* ADMMonster::AddPart(FName Name, UStaticMesh* Shape, const FVector& Location, const FVector& Scale,
	const FLinearColor& Color, float PartRoughness, float PartMetallic, const FRotator& Rotation)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(GetCapsuleComponent());
	Part->SetStaticMesh(Shape);
	Part->SetMaterial(0, PartMaterial);
	Part->SetRelativeLocationAndRotation(Location, Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCanEverAffectNavigation(false);
	Parts.Add(Part);
	PartColors.Add(Color);
	PartGloss.Add(FVector2D(PartRoughness, PartMetallic));
	return Part;
}

void ADMMonster::BeginPlay()
{
	Super::BeginPlay();

	for (int32 i = 0; i < Parts.Num(); ++i)
	{
		if (UMaterialInstanceDynamic* Dynamic = Parts[i]->CreateDynamicMaterialInstance(0))
		{
			Dynamic->SetVectorParameterValue(TEXT("Color"), PartColors[i]);
			Dynamic->SetScalarParameterValue(TEXT("Roughness"), PartGloss[i].X);
			Dynamic->SetScalarParameterValue(TEXT("Metallic"), PartGloss[i].Y);
		}
	}
	StateStartTime = GetWorld()->GetTimeSeconds();
	OnMonsterStateChanged();
}

void ADMMonster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority())
	{
		ServerThink(DeltaSeconds);
	}
}

void ADMMonster::SetMonsterState(EDMMonsterState NewState)
{
	if (MonsterState == NewState)
	{
		return;
	}
	UE_LOG(LogDreamMovers, Log, TEXT("%s: %s -> %s"), *GetName(), *UEnum::GetValueAsString(MonsterState), *UEnum::GetValueAsString(NewState));
	MonsterState = NewState;
	StateStartTime = GetWorld()->GetTimeSeconds();
	OnMonsterStateChanged();
}

void ADMMonster::OnRep_MonsterState()
{
	OnMonsterStateChanged();
}

double ADMMonster::StateAge() const
{
	return GetWorld()->GetTimeSeconds() - StateStartTime;
}

void ADMMonster::MoveToLocation(const FVector& Goal, float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->MoveToLocation(Goal, 40.f, true, true, true, false);
	}
}

void ADMMonster::MoveToActor(AActor* Goal, float Speed)
{
	GetCharacterMovement()->MaxWalkSpeed = Speed;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->MoveToActor(Goal, 20.f, true, true, false);
	}
}

void ADMMonster::StopMoving()
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->StopMovement();
	}
}

bool ADMMonster::IsMoveDone() const
{
	const AAIController* AI = Cast<AAIController>(GetController());
	return !AI || AI->GetMoveStatus() == EPathFollowingStatus::Idle;
}

ADMCharacter* ADMMonster::TryCatch()
{
	const float MyRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	for (TActorIterator<ADMCharacter> It(GetWorld()); It; ++It)
	{
		ADMCharacter* Doll = *It;
		if (Doll->IsDowned() || Doll->IsHiding())
		{
			continue;
		}
		const FVector Delta = Doll->GetActorLocation() - GetActorLocation();
		const float Reach = MyRadius + Doll->GetCapsuleComponent()->GetScaledCapsuleRadius() + CatchRadius;
		if (Delta.SizeSquared2D() <= FMath::Square(Reach) && FMath::Abs(Delta.Z) < 150.f)
		{
			Doll->Down();
			return Doll;
		}
	}
	return nullptr;
}
