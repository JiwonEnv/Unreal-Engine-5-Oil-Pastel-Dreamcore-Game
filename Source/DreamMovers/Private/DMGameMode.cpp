#include "DMGameMode.h"

#include "DMCharacter.h"
#include "DMCottonBall.h"
#include "DMGameState.h"
#include "DMHUD.h"
#include "DMSelfTest.h"
#include "DMValuable.h"
#include "EngineUtils.h"
#include "TimerManager.h"

ADMGameMode::ADMGameMode()
{
	DefaultPawnClass = ADMCharacter::StaticClass();
	GameStateClass = ADMGameState::StaticClass();
	HUDClass = ADMHUD::StaticClass();
}

void ADMGameMode::StartPlay()
{
	Super::StartPlay();

	int32 TotalValue = 0;
	for (TActorIterator<ADMValuable> It(GetWorld()); It; ++It)
	{
		TotalValue += It->GetCurrentValue();
	}
	if (ADMGameState* GS = GetGameState<ADMGameState>())
	{
		GS->StartRound(FMath::RoundToInt(TotalValue * TargetShare), RoundSeconds);
	}
	GetWorldTimerManager().SetTimer(CheckTimer, this, &ADMGameMode::CheckRound, 0.5f, true);

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("dmselftest")))
	{
		GetWorld()->SpawnActor<ADMSelfTest>();
	}
#endif
}

void ADMGameMode::CheckRound()
{
	ADMGameState* GS = GetGameState<ADMGameState>();
	if (!GS || GS->GetRoundState() != EDMRoundState::Playing)
	{
		return;
	}
	if (GS->IsTargetReached())
	{
		GS->EndRound(EDMRoundState::Success);
	}
	else if (GS->GetRemainingSeconds() <= 0.f || AreAllPlayersDown())
	{
		// The dream woke up before enough was banked, or nobody is left to carry anything.
		GS->EndRound(EDMRoundState::Failed);
	}
	else
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(CheckTimer);
	GetWorldTimerManager().SetTimer(RestartTimer, this, &ADMGameMode::RestartRound, SummarySeconds, false);
}

bool ADMGameMode::AreAllPlayersDown() const
{
	// Only once someone has actually been caught, so players still joining do not count as down.
	TActorIterator<ADMCottonBall> AnyBall(GetWorld());
	if (!AnyBall)
	{
		return false;
	}
#if !UE_BUILD_SHIPPING
	// The self test gets its solo doll caught on purpose, to check the revive.
	if (TActorIterator<ADMSelfTest>(GetWorld()))
	{
		return false;
	}
#endif
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (Cast<ADMCharacter>(It->Get()->GetPawn()))
		{
			return false;
		}
	}
	return true;
}

bool ADMGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	// A caught player waits for their cotton ball to be carried out.
	for (TActorIterator<ADMCottonBall> It(GetWorld()); It; ++It)
	{
		if (It->GetDownedController() == Player)
		{
			return false;
		}
	}
	return Super::PlayerCanRestart_Implementation(Player);
}

void ADMGameMode::RevivePlayer(AController* Controller, const FVector& Location)
{
	if (!Controller || Controller->GetPawn())
	{
		return;
	}
	RestartPlayerAtTransform(Controller, FTransform(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f), Location + FVector(0.f, 0.f, 100.f)));
}

void ADMGameMode::RestartRound()
{
	GetWorld()->ServerTravel(TEXT("?Restart"), false);
}
