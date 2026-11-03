#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "DMGameState.generated.h"

UENUM()
enum class EDMRoundState : uint8
{
	Playing,
	Success,
	Failed
};

/** Round score and timer, replicated to every player for the HUD. */
UCLASS()
class DREAMMOVERS_API ADMGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server */
	void StartRound(int32 InTargetValue, float DurationSeconds);
	void AddCollected(int32 Value);
	void AddBroken(int32 Value);
	void EndRound(EDMRoundState Result);

	int32 GetCollectedValue() const { return CollectedValue; }
	int32 GetBrokenValue() const { return BrokenValue; }
	int32 GetTargetValue() const { return TargetValue; }
	EDMRoundState GetRoundState() const { return RoundState; }
	bool IsTargetReached() const { return TargetValue > 0 && CollectedValue >= TargetValue; }
	float GetRemainingSeconds() const;
	float GetRoundDuration() const { return RoundDuration; }

private:
	UPROPERTY(Replicated)
	int32 CollectedValue = 0;

	/** Base value of everything that shattered, shown in the round summary. */
	UPROPERTY(Replicated)
	int32 BrokenValue = 0;

	UPROPERTY(Replicated)
	int32 TargetValue = 0;

	/** Server world time when the dream wakes up; clients compare against GetServerWorldTimeSeconds. */
	UPROPERTY(Replicated)
	double RoundEndTime = 0.0;

	UPROPERTY(Replicated)
	float RoundDuration = 0.f;

	UPROPERTY(Replicated)
	EDMRoundState RoundState = EDMRoundState::Playing;
};
