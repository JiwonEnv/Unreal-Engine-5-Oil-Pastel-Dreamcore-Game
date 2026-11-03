#include "DMExtractionZone.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DMValuable.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ADMExtractionZone::ADMExtractionZone()
{
	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	SetRootComponent(Zone);
	Zone->SetBoxExtent(FVector(150.f, 150.f, 100.f));
	Zone->SetCollisionProfileName(UCollisionProfile::CustomCollisionProfileName);
	Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
	Zone->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	Zone->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Pad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pad"));
	Pad->SetupAttachment(Zone);
	Pad->SetStaticMesh(CylinderMesh.Object);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	Pad->SetMaterial(0, ShapeMaterial.Object);
	Pad->SetRelativeLocation(FVector(0.f, 0.f, -98.f));
	Pad->SetRelativeScale3D(FVector(3.f, 3.f, 0.04f));
	Pad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADMExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInstanceDynamic* PadMaterial = Pad->CreateDynamicMaterialInstance(0))
	{
		PadMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.75f, 0.35f));
	}
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(CheckTimer, this, &ADMExtractionZone::BankItemsInside, CheckInterval, true);
	}
}

void ADMExtractionZone::BankItemsInside()
{
	TArray<AActor*> Inside;
	Zone->GetOverlappingActors(Inside, ADMValuable::StaticClass());
	for (AActor* Actor : Inside)
	{
		ADMValuable* Valuable = CastChecked<ADMValuable>(Actor);
		if (Valuable->GetNumHolders() == 0)
		{
			Valuable->Extract();
		}
	}
}
