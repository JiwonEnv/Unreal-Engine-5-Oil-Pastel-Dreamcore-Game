#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMExtractionZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * The music box bed. Items let go of inside the box are banked; items still on a thread are not,
 * so players can carry through it without cashing in by accident.
 */
UCLASS()
class DREAMMOVERS_API ADMExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	ADMExtractionZone();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction")
	TObjectPtr<UBoxComponent> Zone;

	/** Floor pad so the zone is visible until it has real art. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Extraction")
	TObjectPtr<UStaticMeshComponent> Pad;

	UPROPERTY(EditAnywhere, Category = "Extraction", meta = (ClampMin = "0.05"))
	float CheckInterval = 0.2f;

private:
	void BankItemsInside();

	FTimerHandle CheckTimer;
};
