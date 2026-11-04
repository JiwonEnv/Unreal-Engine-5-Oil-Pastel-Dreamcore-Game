#pragma once

#include "CoreMinimal.h"
#include "PLGTypes.h"

struct PROCLEVELGEN_API FPLGLayoutSettings
{
	TArray<FPLGModuleDef> Modules;
	int32 NumThemes = 0;
	/** General rooms to place before the end room. */
	int32 RoomCount = 4;
	/** Chance to try a corridor before a room at each open door. */
	float CorridorChance = 0.45f;
	/** Every footprint must stay within this distance (cm) of the origin, so one nav bounds volume covers the house. */
	float MaxHalfExtent = 5000.f;
	/** Growth steps per attempt. */
	int32 MaxSteps = 300;
	/** Fresh tries (derived from the seed) before giving up. */
	int32 MaxAttempts = 16;
};

/** Seeded room-module assembly. Pure functions: the same settings and seed give the same layout on every machine. */
namespace PLG
{
	/** Rotates a 2D vector by Quarter x 90 degrees (positive yaw, +X toward +Y). */
	PROCLEVELGEN_API FVector2D Rotate(const FVector2D& V, int32 Quarter);

	/** Socket position in the module's own space (on the footprint edge). */
	PROCLEVELGEN_API FVector2D SocketLocalPoint(const FPLGModuleDef& Def, const FPLGSocket& Socket);
	PROCLEVELGEN_API FVector2D SocketWorldPoint(const FPLGModuleDef& Def, const FPLGPlacedModule& Placed, const FPLGSocket& Socket);
	/** Outward direction of the socket in generator space, as 90 degree turns from +X. */
	PROCLEVELGEN_API int32 SocketWorldQuarter(const FPLGPlacedModule& Placed, const FPLGSocket& Socket);

	PROCLEVELGEN_API FBox2D Footprint(const FPLGModuleDef& Def, const FPLGPlacedModule& Placed);

	/** Grid and socket rule violations, one line each (empty when the catalog follows the spec). */
	PROCLEVELGEN_API TArray<FString> ValidateModules(const TArray<FPLGModuleDef>& Modules, float Grid);

	/**
	 * Start at the origin, the hub on one of its doors, then rooms and corridors on random open doors until
	 * RoomCount rooms are in, then the end room as deep as it fits. Dead-end corridors are removed and rooms get
	 * themes. Retries with derived seeds; returns false (and why in OutReport) if no attempt succeeds.
	 */
	PROCLEVELGEN_API bool GenerateLayout(const FPLGLayoutSettings& Settings, int32 Seed, FPLGLayout& OutLayout, FString& OutReport);

	/**
	 * The 3 + 2 + 3 graybox kit: start, hub, end; two general room frames; straight, corner and T corridors.
	 * Spawn point tags: Item (floor spot for a carried object), HideSmall / HideLarge (against a wall, facing into
	 * the room, for a one or two person hiding spot), Guard (a post for a waiting monster).
	 * General rooms carry a desk prop to crawl under.
	 */
	PROCLEVELGEN_API TArray<FPLGModuleDef> MakeSampleModules();
	PROCLEVELGEN_API TArray<FPLGTheme> MakeSampleThemes();
}
