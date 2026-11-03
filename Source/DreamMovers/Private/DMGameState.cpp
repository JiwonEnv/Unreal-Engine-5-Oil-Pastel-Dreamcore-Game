#include "DMGameState.h"

#include "Net/UnrealNetwork.h"

void ADMGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADMGameState, CollectedValue);
	DOREPLIFETIME(ADMGameState, BrokenValue);
	DOREPLIFETIME(ADMGameState, TargetValue);
	DOREPLIFETIME(ADMGameState, RoundEndTime);
	DOREPLIFETIME(ADMGameState, RoundDuration);
	DOREPLIFETIME(ADMGameState, RoundState);
}

void ADMGameState::StartRound(int32 InTargetValue, float DurationSeconds)
{
	CollectedValue = 0;
	BrokenValue = 0;
	TargetValue = InTargetValue;
	RoundEndTime = GetServerWorldTimeSeconds() + DurationSeconds;
	RoundDuration = DurationSeconds;
	RoundState = EDMRoundState::Playing;
}

void ADMGameState::AddCollected(int32 Value)
{
	if (RoundState == EDMRoundState::Playing)
	{
		CollectedValue += Value;
	}
}

void ADMGameState::AddBroken(int32 Value)
{
	if (RoundState == EDMRoundState::Playing)
	{
		BrokenValue += Value;
	}
}

void ADMGameState::EndRound(EDMRoundState Result)
{
	if (RoundState == EDMRoundState::Playing)
	{
		RoundState = Result;
	}
}

float ADMGameState::GetRemainingSeconds() const
{
	if (RoundState != EDMRoundState::Playing)
	{
		return 0.f;
	}
	return FMath::Max(0.f, float(RoundEndTime - GetServerWorldTimeSeconds()));
}
