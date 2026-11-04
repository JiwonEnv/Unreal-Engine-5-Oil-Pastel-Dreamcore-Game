#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DMGameMode.generated.h"

class APLGGenerator;

/**
 * One dream run: the target is a share of everything on the map that grows with the player count,
 * the timer is the dream's stability. Banking the target wins, running out of stability fails; the round restarts on its own after a short summary.
 */
UCLASS()
class DREAMMOVERS_API ADMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADMGameMode();

	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override;

	/** A cotton ball reached the extraction zone: give its player a fresh doll there. */
	void RevivePlayer(AController* Controller, const FVector& Location);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "10"))
	float RoundSeconds = 720.f;

	/** Target = map value x (BaseTargetShare + TargetSharePerPlayer x players): 30% solo, 60% for four. */
	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "0", ClampMax = "1"))
	float BaseTargetShare = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "0", ClampMax = "1"))
	float TargetSharePerPlayer = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Round", meta = (ClampMin = "1"))
	float SummarySeconds = 8.f;

private:
	/** A generated map comes empty: put the antiques, wardrobes and monsters on its spawn points. */
	void PopulateGeneratedLevel(APLGGenerator& Generator);
	void UpdateTarget(const AController* Leaving = nullptr);
	void CheckRound();
	bool AreAllPlayersDown() const;
	void RestartRound();

	/** Value of every antique placed on the map when the round started. */
	int32 MapValue = 0;

	FTimerHandle CheckTimer;
	FTimerHandle RestartTimer;
};
