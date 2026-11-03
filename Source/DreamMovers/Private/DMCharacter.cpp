#include "DMCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DMCottonBall.h"
#include "DMGrabComponent.h"
#include "DMNoiseSubsystem.h"
#include "DreamMovers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FLinearColor PlayerColors[] = {
		FLinearColor(0.85f, 0.2f, 0.2f),  // mouse: red
		FLinearColor(0.45f, 0.3f, 0.2f),  // bear: brown
		FLinearColor(0.35f, 0.75f, 0.3f), // frog: green
		FLinearColor(0.9f, 0.6f, 0.8f),   // rabbit: pink
	};
}

ADMCharacter::ADMCharacter()
{
	// About 110 cm tall: a chibi doll, not a person.
	GetCapsuleComponent()->InitCapsuleSize(28.f, 55.f);
	BaseEyeHeight = 35.f;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxWalkSpeed = WalkSpeed;
	Move->JumpZVelocity = 380.f;
	Move->AirControl = 0.35f;
	Move->bOrientRotationToMovement = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	// Must match BaseEyeHeight: the server aims the grab from GetPawnViewLocation.
	Camera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(90.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetStaticMesh(CylinderMesh.Object);
	BodyMesh->SetMaterial(0, ShapeMaterial.Object);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -15.f));
	BodyMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.8f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetOwnerNoSee(true);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(GetCapsuleComponent());
	HeadMesh->SetStaticMesh(SphereMesh.Object);
	HeadMesh->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	HeadMesh->SetRelativeScale3D(FVector(0.45f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetOwnerNoSee(true);

	Grab = CreateDefaultSubobject<UDMGrabComponent>(TEXT("Grab"));
}

void ADMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}
	const bool bSprinting = GetCharacterMovement()->IsMovingOnGround() && GetVelocity().Size2D() > WalkSpeed + 30.f;
	FootstepTimer -= DeltaSeconds;
	if (bSprinting && FootstepTimer <= 0.f)
	{
		FootstepTimer = 0.35f;
		UDMNoiseSubsystem::Report(this, GetActorLocation(), SprintNoiseRadius, this);
	}
}

void ADMCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	UDMNoiseSubsystem::Report(this, GetActorLocation(), LandingNoiseRadius, this);
}

void ADMCharacter::Down()
{
	if (!HasAuthority() || bDowned)
	{
		return;
	}
	bDowned = true;
	UE_LOG(LogDreamMovers, Log, TEXT("%s was caught"), *GetName());

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	// Scale goes in the spawn transform: it replaces whatever the constructor set on the root.
	const FTransform BallTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), GetActorLocation(), FVector(0.45f));
	ADMCottonBall* Ball = GetWorld()->SpawnActor<ADMCottonBall>(ADMCottonBall::StaticClass(), BallTransform, Params);

	AController* OwnerController = GetController();
	if (Ball)
	{
		Ball->SetDownedPlayer(OwnerController, GetPlayerState());
	}
	if (OwnerController)
	{
		OwnerController->UnPossess();
	}
	Destroy();

	// The ball's own camera follows it while teammates carry it. Switch a moment later: unpossessing and
	// destroying the doll resets the view, and a remote client needs the ball replicated before the RPC.
	if (APlayerController* PC = Cast<APlayerController>(OwnerController))
	{
		TWeakObjectPtr<ADMCottonBall> WeakBall(Ball);
		FTimerHandle Handle;
		PC->GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(PC, [PC, WeakBall]()
		{
			if (WeakBall.IsValid() && !PC->GetPawn())
			{
				PC->SetViewTarget(WeakBall.Get());
			}
		}), 0.3f, false);
	}
}

void ADMCharacter::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);
	ApplyPlayerColor();
}

void ADMCharacter::ApplyPlayerColor()
{
	const APlayerState* PS = GetPlayerState();
	if (!PS)
	{
		return;
	}
	if (!BodyMaterial)
	{
		BodyMaterial = BodyMesh->CreateDynamicMaterialInstance(0);
		HeadMesh->SetMaterial(0, BodyMaterial);
	}
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), PlayerColors[FMath::Abs(PS->GetPlayerId()) % UE_ARRAY_COUNT(PlayerColors)]);
	}
}

void ADMCharacter::BuildInput()
{
	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_DreamMovers"));

	MoveAction = NewObject<UInputAction>(this, TEXT("IA_DM_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	LookAction = NewObject<UInputAction>(this, TEXT("IA_DM_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;
	JumpAction = NewObject<UInputAction>(this, TEXT("IA_DM_Jump"));
	SprintAction = NewObject<UInputAction>(this, TEXT("IA_DM_Sprint"));
	GrabAction = NewObject<UInputAction>(this, TEXT("IA_DM_Grab"));
	HoldDistanceAction = NewObject<UInputAction>(this, TEXT("IA_DM_HoldDistance"));
	HoldDistanceAction->ValueType = EInputActionValueType::Axis1D;

	// Move is (X = right, Y = forward): W/S go through a swizzle onto Y.
	auto MapMove = [this](FKey Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(MappingContext));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(MappingContext));
		}
	};
	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, false, true);

	MappingContext->MapKey(LookAction, EKeys::Mouse2D);
	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(SprintAction, EKeys::LeftShift);
	MappingContext->MapKey(GrabAction, EKeys::LeftMouseButton);
	MappingContext->MapKey(HoldDistanceAction, EKeys::MouseWheelAxis);
}

void ADMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (!MappingContext)
	{
		BuildInput();
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADMCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADMCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &ADMCharacter::SprintPressed);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &ADMCharacter::SprintReleased);
	Input->BindAction(GrabAction, ETriggerEvent::Started, this, &ADMCharacter::GrabPressed);
	Input->BindAction(GrabAction, ETriggerEvent::Completed, this, &ADMCharacter::GrabReleased);
	Input->BindAction(HoldDistanceAction, ETriggerEvent::Triggered, this, &ADMCharacter::HoldDistance);
}

void ADMCharacter::PawnClientRestart()
{
	// Super builds the input component, which builds the actions the mapping context needs.
	Super::PawnClientRestart();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

void ADMCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void ADMCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>() * LookSensitivity;
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(-Axis.Y);
}

void ADMCharacter::GrabPressed()
{
	Grab->TryGrab();
}

void ADMCharacter::GrabReleased()
{
	Grab->Release();
}

void ADMCharacter::HoldDistance(const FInputActionValue& Value)
{
	Grab->AdjustHoldDistance(Value.Get<float>());
}

void ADMCharacter::SprintPressed()
{
	SetSprinting(true);
}

void ADMCharacter::SprintReleased()
{
	SetSprinting(false);
}

void ADMCharacter::SetSprinting(bool bSprinting)
{
	// Set on both sides so client prediction and the server agree on the speed.
	GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed;
	if (!HasAuthority())
	{
		ServerSetSprinting(bSprinting);
	}
}

void ADMCharacter::ServerSetSprinting_Implementation(bool bSprinting)
{
	SetSprinting(bSprinting);
}
