#pragma once

#include "CoreMinimal.h"
#include "DMMonster.h"
#include "DMBellHorse.generated.h"

/**
 * Bell Horse: blind, hunts by sound. It stands at its post until a noise reaches it (a bumped antique,
 * sprinting, the butler's bell), then charges to where the sound came from, faster than a sprinting doll.
 * There it searches for a few seconds and walks back. A doll that keeps still is safe even right beside it.
 * Its own bells jingle while it runs, which is the players' warning (shown on the HUD until there is audio).
 */
UCLASS()
class DREAMMOVERS_API ADMBellHorse : public ADMMonster
{
	GENERATED_BODY()

public:
	ADMBellHorse();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ServerThink(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Bell Horse")
	float ChargeSpeed = 600.f;

	UPROPERTY(EditAnywhere, Category = "Bell Horse")
	float ReturnSpeed = 170.f;

	UPROPERTY(EditAnywhere, Category = "Bell Horse")
	float SearchSeconds = 3.f;

	/** Noises closer than this to the horse are ignored, so it does not chase its own clatter. */
	UPROPERTY(EditAnywhere, Category = "Bell Horse")
	float IgnoreNoiseWithin = 250.f;

private:
	void OnNoise(const FVector& Location, float Radius, AActor* Source);

	FVector Post = FVector::ZeroVector;
	FVector NoiseLocation = FVector::ZeroVector;
	FDelegateHandle NoiseHandle;
};
