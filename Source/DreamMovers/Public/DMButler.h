#pragma once

#include "CoreMinimal.h"
#include "DMMonster.h"
#include "DMButler.generated.h"

class USpotLightComponent;

/**
 * Candlestick Butler: walks a patrol loop, and its candle is a real spotlight that is also its sight.
 * A doll that stays in the light (with line of sight) for SpotSeconds is spotted: the butler rings
 * its hand bell, which the bell horse hears from far away, and follows slower than a walking doll.
 * Hiding behind furniture or a carried antique breaks line of sight.
 */
UCLASS()
class DREAMMOVERS_API ADMButler : public ADMMonster
{
	GENERATED_BODY()

public:
	ADMButler();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Before BeginPlay (deferred spawn): loop of points relative to the spawn transform. */
	void SetPatrolPoints(const TArray<FVector>& RelativePoints) { PatrolPoints = RelativePoints; }

	/** 0..1, how close it is to spotting someone. Drives the "?" over its head. */
	float GetSuspicion() const { return Suspicion / 255.f; }

protected:
	virtual void BeginPlay() override;
	virtual void ServerThink(float DeltaSeconds) override;
	virtual void OnMonsterStateChanged() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Butler")
	TObjectPtr<USpotLightComponent> Candlelight;

	/** Loop of points relative to where the butler is placed. Drag them in the viewport. */
	UPROPERTY(EditAnywhere, Category = "Butler", meta = (MakeEditWidget))
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float SightRange = 1000.f;

	/** Half angle of the light cone, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Butler")
	float SightHalfAngle = 32.f;

	/** Seconds in the light before being spotted (faster up close). */
	UPROPERTY(EditAnywhere, Category = "Butler")
	float SpotSeconds = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float LoseSightSeconds = 3.f;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float PatrolSpeed = 140.f;

	/** Slower than a walking doll (330): being seen is a chase you can win. */
	UPROPERTY(EditAnywhere, Category = "Butler")
	float ChaseSpeed = 270.f;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float BellRadius = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float BellInterval = 5.f;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float PausePerPoint = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Butler")
	float SearchSeconds = 3.f;

private:
	ADMCharacter* FindVisibleDoll(float& OutDistance) const;
	bool CanSee(const ADMCharacter* Doll, float* OutDistance = nullptr) const;
	void RingBell();
	void ResumePatrol();

	UPROPERTY(Replicated)
	uint8 Suspicion = 0;

	TArray<FVector> WorldPatrol;
	int32 PatrolIndex = 0;
	double PauseUntil = 0.0;
	float SpotProgress = 0.f;

	TWeakObjectPtr<ADMCharacter> Target;
	FVector LastKnownLocation = FVector::ZeroVector;
	double LastSeenTime = 0.0;
	double LastBellTime = -100.0;
	double LastRepathTime = 0.0;
};
