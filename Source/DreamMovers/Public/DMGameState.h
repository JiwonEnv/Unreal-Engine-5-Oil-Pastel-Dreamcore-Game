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

/**
 * Round score and dream stability, replicated to every player for the HUD.
 * Stability is the time left until the dream wakes: it runs down on its own and every banked antique drains more.
 */
UCLASS()
class DREAMMOVERS_API ADMGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server */
	void StartRound(int32 InTargetValue, float DurationSeconds);
	void SetTargetValue(int32 InTargetValue);
	/** Banks Value and drains DrainSecondsPer1000 of stability for every 1,000 of it. */
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
	/** 1 at the start of the round, 0 when the dream wakes. */
	float GetStability() const { return RoundDuration > 0.f ? GetRemainingSeconds() / RoundDuration : 0.f; }

protected:
	/** Seconds of stability lost per 1,000 value banked: taking memories out makes the dream fade faster. */
	UPROPERTY(EditDefaultsOnly, Category = "Dream", meta = (ClampMin = "0"))
	float DrainSecondsPer1000 = 20.f;

private:
	UPROPERTY(Replicated)
	int32 CollectedValue = 0;

	/** Base value of everything that shattered, shown in the round summary. */
	UPROPERTY(Replicated)
	int32 BrokenValue = 0;

	UPROPERTY(Replicated)
	int32 TargetValue = 0;

	/** Server world time when the dream wakes up; clients compare against GetServerWorldTimeSeconds. Banking pulls it earlier. */
	UPROPERTY(Replicated)
	double RoundEndTime = 0.0;

	UPROPERTY(Replicated)
	float RoundDuration = 0.f;

	UPROPERTY(Replicated)
	EDMRoundState RoundState = EDMRoundState::Playing;
};
