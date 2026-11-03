#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DMGrabComponent.generated.h"

class ADMValuable;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * The "thread" grab: the owning client picks a target and the server pulls it with a capped spring force.
 *
 * Forces from several holders simply add up, so an item heavier than MaxLiftKg drags along the floor
 * for one player and lifts once a second player grabs it. The server reads the aim from the pawn's
 * replicated control rotation, so only grab, release and hold-distance changes go over the network.
 */
UCLASS(ClassGroup = (DreamMovers), meta = (BlueprintSpawnableComponent))
class DREAMMOVERS_API UDMGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDMGrabComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Local player input */
	void TryGrab();
	void Release();
	void AdjustHoldDistance(float Direction);

	ADMValuable* GetHeldValuable() const { return HeldValuable; }
	float GetMaxLiftKg() const { return MaxLiftKg; }

	/** What the local player is aiming at within grab range, for the HUD. */
	ADMValuable* TraceForValuable(FVector* OutHitPoint = nullptr) const;

protected:
	UPROPERTY(EditAnywhere, Category = "Grab")
	float GrabRange = 400.f;

	UPROPERTY(EditAnywhere, Category = "Grab")
	float MinHoldDistance = 90.f;

	UPROPERTY(EditAnywhere, Category = "Grab")
	float MaxHoldDistance = 400.f;

	/** Mouse wheel step for pulling the item closer or pushing it away. */
	UPROPERTY(EditAnywhere, Category = "Grab")
	float HoldDistanceStep = 25.f;

	/** Strength: the heaviest item one player can hold up against gravity. */
	UPROPERTY(EditAnywhere, Category = "Grab")
	float MaxLiftKg = 20.f;

	/** Spring frequency (rad/s): higher snaps to the aim point faster but jitters more over the network. */
	UPROPERTY(EditAnywhere, Category = "Grab")
	float SpringFrequency = 9.f;

	UPROPERTY(EditAnywhere, Category = "Grab", meta = (ClampMin = "0"))
	float SpringDampingRatio = 0.9f;

	/** The thread snaps if the item ends up this far past the hold point (stuck behind a wall). */
	UPROPERTY(EditAnywhere, Category = "Grab")
	float BreakDistance = 250.f;

	UPROPERTY(EditAnywhere, Category = "Grab")
	FLinearColor ThreadColor = FLinearColor(1.f, 0.35f, 0.55f);

private:
	UFUNCTION(Server, Reliable)
	void ServerGrab(ADMValuable* Target, FVector_NetQuantize WorldGrabPoint);

	UFUNCTION(Server, Reliable)
	void ServerRelease();

	UFUNCTION(Server, Reliable)
	void ServerSetHoldDistance(float NewDistance);

	void ServerApplyForce(float DeltaTime);
	void ServerDropHeld();
	void UpdateThreadVisual();

	FVector GetViewLocation() const;
	FVector GetAimDirection() const;
	FVector GetThreadStart() const;

	UPROPERTY(Replicated)
	TObjectPtr<ADMValuable> HeldValuable;

	/** Grab point in the item's local space, replicated so every client draws the thread to the same spot. */
	UPROPERTY(Replicated)
	FVector_NetQuantize100 LocalGrabOffset;

	float HoldDistance = 200.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> ThreadMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ThreadMaterial;
};
