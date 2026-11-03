#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DMGameMode.generated.h"

/**
 * One dream run: the target is a share of everything on the map, the timer is the dream's stability.
 * Banking the target wins, running out of time fails; the round restarts on its own after a short summary.
 */
UCLASS()
class DREAMMOVERS_API ADMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADMGameMode();

	virtual void StartPlay() override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	/** A cotton ball reached the extraction zone: give its player a fresh doll there. */
	void RevivePlayer(AController* Controller, const FVector& Location);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "10"))
	float RoundSeconds = 720.f;

	/** Share of the total value placed on the map that must be extracted. */
	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "0.05", ClampMax = "1"))
	float TargetShare = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "1"))
	float SummarySeconds = 8.f;

private:
	void CheckRound();
	bool AreAllPlayersDown() const;
	void RestartRound();

	FTimerHandle CheckTimer;
	FTimerHandle RestartTimer;
};
