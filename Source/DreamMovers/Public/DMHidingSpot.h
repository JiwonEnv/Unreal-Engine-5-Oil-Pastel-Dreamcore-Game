#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMHidingSpot.generated.h"

class ADMCharacter;
class UStaticMeshComponent;

/**
 * A wardrobe a doll can climb into. Inside, the butler's candlelight cannot find you and nothing can catch you,
 * but the doors creak on the way in and out, and the bell horse hears that. Empty hands only.
 * The root sits on the floor; the front (the side you step out of) faces +X.
 */
UCLASS()
class DREAMMOVERS_API ADMHidingSpot : public AActor
{
	GENERATED_BODY()

public:
	ADMHidingSpot();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server: puts Doll inside if there is room. */
	bool TryEnter(ADMCharacter* Doll);
	/** Server: lets Doll out in front of the doors. */
	void Leave(ADMCharacter* Doll);

	bool HasRoom() const { return NumOccupants < Capacity; }
	int32 GetNumOccupants() const { return NumOccupants; }
	int32 GetCapacity() const { return Capacity; }

	/** Closest wardrobe Doll can reach from where it stands, or null. */
	static ADMHidingSpot* FindReachable(const ADMCharacter* Doll);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hiding")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Outer size in cm (depth, width, height). */
	UPROPERTY(EditAnywhere, Category = "Hiding")
	FVector Size = FVector(70.f, 130.f, 200.f);

	/** One doll for a narrow wardrobe, two for a wide one. */
	UPROPERTY(EditAnywhere, Category = "Hiding", meta = (ClampMin = "1", ClampMax = "2"))
	int32 Capacity = 2;

	/** The doors creak this far (cm) when someone gets in or out. */
	UPROPERTY(EditAnywhere, Category = "Hiding")
	float DoorNoiseRadius = 900.f;

	/** How close (cm, from the front face) a doll must stand to get in. */
	UPROPERTY(EditAnywhere, Category = "Hiding")
	float ReachDistance = 120.f;

private:
	FVector GetSlotLocation(int32 Slot) const;
	FVector GetExitLocation(int32 Slot) const;
	void PurgeOccupants();

	TArray<TWeakObjectPtr<ADMCharacter>> Occupants;

	/** For the HUD prompt. */
	UPROPERTY(Replicated)
	int32 NumOccupants = 0;
};
