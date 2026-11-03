#include "DMNoiseSubsystem.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<bool> CVarShowNoise(TEXT("dm.ShowNoise"), false, TEXT("Draw noise events (server only)."));
}

void UDMNoiseSubsystem::Report(const UObject* WorldContext, const FVector& Location, float Radius, AActor* Source)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World || World->GetNetMode() == NM_Client)
	{
		return;
	}
	if (CVarShowNoise.GetValueOnGameThread())
	{
		DrawDebugSphere(World, Location, Radius, 16, FColor::Orange, false, 0.6f);
	}
	if (UDMNoiseSubsystem* Noise = World->GetSubsystem<UDMNoiseSubsystem>())
	{
		Noise->OnNoise.Broadcast(Location, Radius, Source);
	}
}
