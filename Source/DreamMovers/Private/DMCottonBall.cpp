#include "DMCottonBall.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DMGameMode.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "DreamMoversCotton"

ADMCottonBall::ADMCottonBall()
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	Mesh->SetStaticMesh(SphereMesh.Object);

	BaseValue = 0;
	MassKg = 3.f;
	CrackSpeed = 100000.f; // never cracks
	QuietSpeed = 100000.f; // and lands softly
	Tint = FLinearColor(0.96f, 0.94f, 0.9f);
	Roughness = 0.95f;

	// Absolute rotation so the view does not tumble with the rolling ball.
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(Mesh);
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->SetUsingAbsoluteScale(true);
	CameraArm->SetWorldRotation(FRotator(-35.f, 0.f, 0.f));
	CameraArm->TargetArmLength = 320.f;
	CameraArm->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm);
}

void ADMCottonBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADMCottonBall, DownedPlayer);
}

FText ADMCottonBall::GetDisplayName() const
{
	if (DownedPlayer)
	{
		return FText::Format(LOCTEXT("NamedBall", "{0}의 솜뭉치"), FText::FromString(DownedPlayer->GetPlayerName()));
	}
	return LOCTEXT("Ball", "솜뭉치");
}

void ADMCottonBall::SetDownedPlayer(AController* InController, APlayerState* InPlayerState)
{
	DownedController = InController;
	DownedPlayer = InPlayerState;
	// Look the way the doll was facing when it was caught.
	CameraArm->SetWorldRotation(FRotator(-35.f, GetActorRotation().Yaw, 0.f));
}

void ADMCottonBall::Extract()
{
	if (!HasAuthority() || IsExtracted())
	{
		return;
	}
	if (ADMGameMode* GameMode = GetWorld()->GetAuthGameMode<ADMGameMode>())
	{
		GameMode->RevivePlayer(DownedController.Get(), GetActorLocation());
	}
	// Skip ADMValuable::Extract: a revived friend is not loot.
	MarkExtracted();
	Destroy();
}

#undef LOCTEXT_NAMESPACE
