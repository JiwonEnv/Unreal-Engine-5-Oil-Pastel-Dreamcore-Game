#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMSelfTest.generated.h"

class ADMCharacter;

/**
 * Development-only smoke test of Lvl_Proto, started with `-dmselftest` (never in Shipping).
 * Drives the listen server's own doll through grab, noise, butler and revive checks, logs
 * "[SelfTest] PASS/FAIL" lines on LogDreamMovers, takes screenshots, and quits.
 */
UCLASS(NotPlaceable)
class DREAMMOVERS_API ADMSelfTest : public AActor
{
	GENERATED_BODY()

public:
	ADMSelfTest();

	virtual void Tick(float DeltaSeconds) override;

private:
	void Step(int32 Index);
	ADMCharacter* Doll() const;
	AActor* FindValuable(const TCHAR* NamePrefix) const;
	bool PlaceDollFacing(const FVector& Location, const FVector& LookAt);
	/** Players look up after grabbing: that is what lifts the item to eye height. */
	void LookLevel();
	void Check(bool bPassed, const FString& What);
	void Shot(const TCHAR* Name);

	int32 NextStep = 0;
	double StepTime = 0.0;
	int32 Passed = 0;
	int32 Failed = 0;

	TWeakObjectPtr<AActor> LightItem;
	TWeakObjectPtr<AActor> HeavyItem;
	float LightItemStartZ = 0.f;
	float HeavyItemStartZ = 0.f;
};
