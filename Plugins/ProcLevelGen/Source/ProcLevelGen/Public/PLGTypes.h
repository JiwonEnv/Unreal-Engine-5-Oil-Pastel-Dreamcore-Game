#pragma once

#include "CoreMinimal.h"
#include "PLGTypes.generated.h"

/**
 * Module spec (Docs/ModuleSpec.md):
 * - Pivot: center of the footprint, on the top of the floor (Z = 0). Modules only turn in 90 degree steps around it.
 * - Footprint sizes are multiples of 100 cm, so the pivot always lands on the 50 cm grid.
 * - Each module owns half of the wall (10 of 20 cm) inside its footprint; two modules side by side make one 20 cm wall.
 * - A door socket is a point on the footprint edge facing out. Two sockets connect when they meet, face each other
 *   and have the same width. Sockets left unconnected stay solid wall.
 */

UENUM(BlueprintType)
enum class EPLGRole : uint8
{
	/** Placed first, at the generator's origin. */
	Start,
	/** Placed right on a Start socket. */
	Hub,
	/** Placed last, as deep in the layout as it fits. */
	End,
	/** General room: reused frames, each gets a theme. */
	Room,
	/** Connects rooms; dead-end corridors are removed. */
	Corridor
};

/** Footprint edge in the module's own space; the value is also the number of 90 degree turns of its outward normal from +X. */
UENUM(BlueprintType)
enum class EPLGEdge : uint8
{
	PosX,
	PosY,
	NegX,
	NegY
};

USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGSocket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	EPLGEdge Edge = EPLGEdge::PosX;

	/** cm from the edge's center along the edge (toward +Y on X edges, +X on Y edges). Multiple of the grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	float Offset = 0.f;

	/** Door width in cm: 150 small door, 300 double door. Only equal widths connect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
	float Width = 150.f;
};

/** A place in a module where the game may put content; what goes there is up to the game, by tag. */
USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGSpawnPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FName Tag;

	/** Module space (cm), Z = 0 is the floor top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	FVector Location = FVector::ZeroVector;

	/** Facing in module space, degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float Yaw = 0.f;
};

/** Static furniture built with the graybox on every machine, so it needs no replication (desks, shelves). */
USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGPropBox
{
	GENERATED_BODY()

	/** Module space (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Prop")
	FVector Size = FVector(100.f);
};

USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGModuleDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	EPLGRole Role = EPLGRole::Room;

	/** Footprint in cm (X, Y), walls included. Multiples of 100. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	FVector2D Size = FVector2D(800.f, 800.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	TArray<FPLGSocket> Sockets;

	/** Relative chance against other modules with the same role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module", meta = (ClampMin = "0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	TArray<FPLGSpawnPoint> SpawnPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	TArray<FPLGPropBox> Props;
};

/** Look of a general room. Graybox only tints; real themes will swap wall and floor materials and contents. */
USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGTheme
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
	FLinearColor FloorColor = FLinearColor(0.5f, 0.4f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme")
	FLinearColor WallColor = FLinearColor(0.9f, 0.85f, 0.75f);
};

USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGPlacedModule
{
	GENERATED_BODY()

	/** Index into the generator's module list. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 Def = INDEX_NONE;

	/** Pivot in the generator's space (cm). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	FVector2D Location = FVector2D::ZeroVector;

	/** Yaw in 90 degree steps (0..3). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 Quarter = 0;

	/** Doors away from the start module. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 Depth = 0;

	/** Index into the theme list, rooms only. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 Theme = INDEX_NONE;

	/** Per socket of the def: the placed module behind that door, or INDEX_NONE when it stays wall. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	TArray<int32> Links;

	/** Pruned (a dead-end corridor); skipped when building. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	bool bRemoved = false;
};

/** A spawn point resolved into the world for one placed module. */
USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGSpawnSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	FTransform Transform;

	/** Index into the layout's modules. */
	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	int32 Module = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	EPLGRole Role = EPLGRole::Room;

	UPROPERTY(BlueprintReadOnly, Category = "Spawn")
	int32 Theme = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct PROCLEVELGEN_API FPLGLayout
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	int32 Seed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Layout")
	TArray<FPLGPlacedModule> Modules;
};
