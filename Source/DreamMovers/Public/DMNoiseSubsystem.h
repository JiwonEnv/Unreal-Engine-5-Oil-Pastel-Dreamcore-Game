#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DMNoiseSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FDMOnNoise, const FVector& /*Location*/, float /*Radius*/, AActor* /*Source*/);

/**
 * Server-side noise events: bumped antiques, sprinting dolls, the butler's bell.
 * The bell horse is blind and hunts only by these. `dm.ShowNoise 1` draws them on the server.
 */
UCLASS()
class DREAMMOVERS_API UDMNoiseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Anything within Radius (cm) of Location can hear it. Ignored on clients. */
	static void Report(const UObject* WorldContext, const FVector& Location, float Radius, AActor* Source);

	FDMOnNoise OnNoise;
};
