#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DMCharacter.generated.h"

class UCameraComponent;
class UDMGrabComponent;
class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * First-person doll. Small capsule (the dolls are about 2.5 heads tall), a placeholder body that
 * only other players see, and the thread grab. Input is built in code so no input assets are needed.
 */
UCLASS()
class DREAMMOVERS_API ADMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADMCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;

	UDMGrabComponent* GetGrab() const { return Grab; }

	/** Server: caught by a monster. The doll bursts into a cotton ball that teammates can carry to the extraction zone. */
	void Down();
	bool IsDowned() const { return bDowned; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doll")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doll")
	TObjectPtr<UDMGrabComponent> Grab;

	/** Stand-in until the Tripo dolls exist: a colored body and head. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doll")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Doll")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Doll")
	float WalkSpeed = 330.f;

	UPROPERTY(EditDefaultsOnly, Category = "Doll")
	float SprintSpeed = 520.f;

	UPROPERTY(EditDefaultsOnly, Category = "Doll")
	float LookSensitivity = 0.4f;

	/** Sprinting footsteps are heard this far (cm); walking is silent. */
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	float SprintNoiseRadius = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	float LandingNoiseRadius = 500.f;

private:
	void BuildInput();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void GrabPressed();
	void GrabReleased();
	void HoldDistance(const FInputActionValue& Value);
	void SprintPressed();
	void SprintReleased();
	void SetSprinting(bool bSprinting);

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bSprinting);

	void ApplyPlayerColor();

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> GrabAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> HoldDistanceAction;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	bool bDowned = false;
	float FootstepTimer = 0.f;
};
