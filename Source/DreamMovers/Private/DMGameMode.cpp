#include "DMGameMode.h"

#include "DMBellHorse.h"
#include "DMButler.h"
#include "DMCharacter.h"
#include "DMCottonBall.h"
#include "DMGameState.h"
#include "DMHidingSpot.h"
#include "DMHUD.h"
#include "DMSelfTest.h"
#include "DMValuable.h"
#include "DMValuableCatalog.h"
#include "DreamMovers.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "PLGGenerator.h"
#include "TimerManager.h"

namespace
{
	template <typename T>
	void Shuffle(TArray<T>& Array, FRandomStream& Rng)
	{
		for (int32 i = Array.Num() - 1; i > 0; --i)
		{
			Array.Swap(i, Rng.RandRange(0, i));
		}
	}

	/** The butler starts in the chapel (GDD 6.3), or the deepest room if this layout has none. */
	const FName ButlerTheme(TEXT("Chapel"));
}

ADMGameMode::ADMGameMode()
{
	DefaultPawnClass = ADMCharacter::StaticClass();
	GameStateClass = ADMGameState::StaticClass();
	HUDClass = ADMHUD::StaticClass();
}

void ADMGameMode::StartPlay()
{
	Super::StartPlay();

	// The generator built its layout during Super::StartPlay (BeginPlay); content goes in before the target is counted.
	if (TActorIterator<APLGGenerator> Generator(GetWorld()); Generator && Generator->GetLayout().Modules.Num() > 0)
	{
		PopulateGeneratedLevel(**Generator);
	}

	MapValue = 0;
	for (TActorIterator<ADMValuable> It(GetWorld()); It; ++It)
	{
		MapValue += It->GetCurrentValue();
	}
	if (ADMGameState* GS = GetGameState<ADMGameState>())
	{
		GS->StartRound(0, RoundSeconds);
	}
	UpdateTarget();
	GetWorldTimerManager().SetTimer(CheckTimer, this, &ADMGameMode::CheckRound, 0.5f, true);

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("dmselftest")))
	{
		GetWorld()->SpawnActor<ADMSelfTest>();
	}
#endif
}

void ADMGameMode::PopulateGeneratedLevel(APLGGenerator& Generator)
{
	UWorld* World = GetWorld();
	FRandomStream Rng = Generator.MakeContentStream();

	// Antiques: the chapter's set, each once, on shuffled floor spots.
	TArray<FPLGSpawnSlot> ItemSlots = Generator.GetSpawnSlots(TEXT("Item"));
	TArray<FDMValuableSpec> Specs = DMValuableCatalog::MakeChapterOne();
	Shuffle(ItemSlots, Rng);
	Shuffle(Specs, Rng);
	const int32 NumItems = FMath::Min(ItemSlots.Num(), Specs.Num());
	for (int32 i = 0; i < NumItems; ++i)
	{
		const FDMValuableSpec& Spec = Specs[i];
		FTransform Transform = ItemSlots[i].Transform;
		Transform.SetRotation(FQuat(FRotator(0.f, Rng.FRandRange(0.f, 360.f), 0.f)) * Transform.GetRotation());
		Transform.AddToTranslation(FVector(0.f, 0.f, Spec.Size.Z * 0.5f + 1.f));
		Transform.SetScale3D(Spec.Size / 100.f);
		if (ADMValuable* Item = World->SpawnActorDeferred<ADMValuable>(ADMValuable::StaticClass(), Transform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
		{
			Item->InitFromSpec(Spec);
			Item->FinishSpawning(Transform);
		}
	}

	// Wardrobes on every hiding spot: narrow ones for one doll, wide ones for two.
	int32 NumWardrobes = 0;
	struct FWardrobeKind
	{
		const TCHAR* Tag;
		FVector Size;
		int32 Capacity;
	};
	const FWardrobeKind Wardrobes[] = {
		{ TEXT("HideSmall"), FVector(70.f, 70.f, 180.f), 1 },
		{ TEXT("HideLarge"), FVector(70.f, 130.f, 200.f), 2 },
	};
	for (const FWardrobeKind& Kind : Wardrobes)
	{
		for (const FPLGSpawnSlot& Slot : Generator.GetSpawnSlots(Kind.Tag))
		{
			if (ADMHidingSpot* Spot = World->SpawnActorDeferred<ADMHidingSpot>(ADMHidingSpot::StaticClass(), Slot.Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
			{
				Spot->Configure(Kind.Size, Kind.Capacity);
				Spot->FinishSpawning(Slot.Transform);
				++NumWardrobes;
			}
		}
	}

	// Bell horse waits on the hub's post.
	const TArray<FPLGSpawnSlot> Posts = Generator.GetSpawnSlots(TEXT("Guard"));
	if (Posts.Num() > 0)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		World->SpawnActor<ADMBellHorse>(ADMBellHorse::StaticClass(), Posts[0].Transform.GetLocation() + FVector(0.f, 0.f, 90.f),
			Posts[0].Transform.Rotator(), Params);
	}

	// Butler: starts in the chapel and walks a loop through every general room and the hub.
	const FPLGLayout& Layout = Generator.GetLayout();
	const TArray<FPLGModuleDef>& Defs = Generator.GetModules();
	const TArray<FPLGTheme>& Themes = Generator.GetThemes();
	TArray<int32> Route;
	int32 ButlerRoom = INDEX_NONE;
	for (int32 i = 0; i < Layout.Modules.Num(); ++i)
	{
		const FPLGPlacedModule& Placed = Layout.Modules[i];
		if (Placed.bRemoved)
		{
			continue;
		}
		const EPLGRole ModuleRole = Defs[Placed.Def].Role;
		if (ModuleRole == EPLGRole::Room || ModuleRole == EPLGRole::Hub)
		{
			Route.Add(i);
		}
		if (ModuleRole == EPLGRole::Room)
		{
			const bool bChapel = Themes.IsValidIndex(Placed.Theme) && Themes[Placed.Theme].Name == ButlerTheme;
			const bool bDeeper = ButlerRoom == INDEX_NONE || Placed.Depth > Layout.Modules[ButlerRoom].Depth;
			const bool bCurrentIsChapel = ButlerRoom != INDEX_NONE && Themes.IsValidIndex(Layout.Modules[ButlerRoom].Theme)
				&& Themes[Layout.Modules[ButlerRoom].Theme].Name == ButlerTheme;
			if (bChapel || (!bCurrentIsChapel && bDeeper))
			{
				ButlerRoom = i;
			}
		}
	}
	if (ButlerRoom != INDEX_NONE)
	{
		// Start the loop at the butler's own room.
		const int32 First = Route.IndexOfByKey(ButlerRoom);
		TArray<int32> Ordered;
		for (int32 k = 0; k < Route.Num(); ++k)
		{
			Ordered.Add(Route[(First + k) % Route.Num()]);
		}
		const FVector Origin = Generator.GetModuleTransform(ButlerRoom).GetLocation() + FVector(0.f, 0.f, 100.f);
		TArray<FVector> Patrol;
		for (int32 Index : Ordered)
		{
			Patrol.Add(Generator.GetModuleTransform(Index).GetLocation() + FVector(0.f, 0.f, 100.f) - Origin);
		}
		// Unrotated: the patrol points are relative to the spawn transform.
		const FTransform ButlerTransform(FRotator::ZeroRotator, Origin);
		if (ADMButler* Butler = World->SpawnActorDeferred<ADMButler>(ADMButler::StaticClass(), ButlerTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn))
		{
			Butler->SetPatrolPoints(Patrol);
			Butler->FinishSpawning(ButlerTransform);
		}
	}

	UE_LOG(LogDreamMovers, Log, TEXT("Generated level (seed %d): %d antiques on %d spots, %d wardrobes, horse %s, butler patrols %d rooms"),
		Generator.GetSeed(), NumItems, ItemSlots.Num(), NumWardrobes, Posts.Num() > 0 ? TEXT("yes") : TEXT("no"), ButlerRoom != INDEX_NONE ? Route.Num() : 0);

#if !UE_BUILD_SHIPPING
	// -plgselftest: once physics settled, every antique should rest on a floor inside the house.
	if (FParse::Param(FCommandLine::Get(), TEXT("plgselftest")))
	{
		FTimerHandle Handle;
		GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			int32 Items = 0;
			int32 Resting = 0;
			for (TActorIterator<ADMValuable> It(GetWorld()); It; ++It)
			{
				++Items;
				const float Z = It->GetActorLocation().Z;
				Resting += (Z > 0.f && Z < 300.f && It->GetVelocity().Size() < 20.f) ? 1 : 0;
			}
			int32 Spots = 0;
			for (TActorIterator<ADMHidingSpot> It(GetWorld()); It; ++It)
			{
				++Spots;
			}
			int32 Butlers = 0;
			int32 Horses = 0;
			for (TActorIterator<ADMButler> It(GetWorld()); It; ++It)
			{
				Butlers += It->GetController() ? 1 : 0;
			}
			for (TActorIterator<ADMBellHorse> It(GetWorld()); It; ++It)
			{
				Horses += It->GetController() ? 1 : 0;
			}
			const ADMGameState* GS = GetGameState<ADMGameState>();
			const bool bPassed = Items > 0 && Resting == Items && Spots > 0 && Butlers == 1 && Horses == 1 && GS && GS->GetTargetValue() > 0;
			UE_LOG(LogDreamMovers, Display, TEXT("[DMProcTest] %s %d/%d antiques resting on the floor, %d wardrobes, butler %d, horse %d (AI possessed), target %d of %d"),
				bPassed ? TEXT("PASS") : TEXT("FAIL"), Resting, Items, Spots, Butlers, Horses, GS ? GS->GetTargetValue() : 0, MapValue);
		}), 7.5f, false);
	}
#endif
}

void ADMGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UpdateTarget();
}

void ADMGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UpdateTarget(Exiting);
}

void ADMGameMode::UpdateTarget(const AController* Leaving)
{
	ADMGameState* GS = GetGameState<ADMGameState>();
	if (!GS || MapValue <= 0)
	{
		return;
	}
	int32 Players = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get() != Leaving)
		{
			++Players;
		}
	}
	const float Share = FMath::Min(1.f, BaseTargetShare + TargetSharePerPlayer * FMath::Max(1, Players));
	GS->SetTargetValue(FMath::RoundToInt(MapValue * Share));
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
		// The dream woke up (time and banking drained it) before enough was banked, or nobody is left to carry anything.
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
