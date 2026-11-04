#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "PLGTypes.h"
#include "PLGGenerator.generated.h"

class APLGGenerator;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPLGOnGenerated, APLGGenerator*, Generator);

/**
 * Drop one in a level: on BeginPlay it assembles the module kit from a seed and builds graybox floors and walls.
 * The server picks the seed and replicates it; every machine builds the same layout locally, nothing else is sent.
 * In the editor, "Preview" and "Preview New Seed" build the same thing without playing (not saved with the level).
 */
UCLASS()
class PROCLEVELGEN_API APLGGenerator : public AActor
{
	GENERATED_BODY()

public:
	APLGGenerator();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Builds the layout for the current seed. */
	UFUNCTION(CallInEditor, BlueprintCallable, Category = "Generation")
	void Preview();

	UFUNCTION(CallInEditor, Category = "Generation")
	void PreviewNewSeed();

	UFUNCTION(CallInEditor, Category = "Generation")
	void ClearPreview();

	const FPLGLayout& GetLayout() const { return Layout; }
	const TArray<FPLGModuleDef>& GetModules() const { return Modules; }
	const TArray<FPLGTheme>& GetThemes() const { return Themes; }
	int32 GetSeed() const { return Seed; }

	/** Every spawn point with this tag in the built layout, in world space. */
	UFUNCTION(BlueprintPure, Category = "Generation")
	TArray<FPLGSpawnSlot> GetSpawnSlots(FName Tag) const;

	/** Random stream for content choices, derived from the seed (the server uses it; clients get the results replicated). */
	FRandomStream MakeContentStream() const;

	/** World transform of a placed module's pivot (floor top, footprint center). */
	UFUNCTION(BlueprintPure, Category = "Generation")
	FTransform GetModuleTransform(int32 PlacedIndex) const;

	/** Fired on every machine after a layout is built. Content spawning hooks in here. */
	UPROPERTY(BlueprintAssignable, Category = "Generation")
	FPLGOnGenerated OnGenerated;

protected:
	/** 0 or bNewSeedEachRun: the server picks one when play starts. */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Seed, Category = "Generation")
	int32 Seed = 1;

	UPROPERTY(EditAnywhere, Category = "Generation")
	bool bNewSeedEachRun = true;

	/** General rooms placed before the end room. */
	UPROPERTY(EditAnywhere, Category = "Generation", meta = (ClampMin = "0", ClampMax = "32"))
	int32 RoomCount = 4;

	UPROPERTY(EditAnywhere, Category = "Generation", meta = (ClampMin = "0", ClampMax = "1"))
	float CorridorChance = 0.45f;

	/** Modules stay within this distance (cm) of the generator, so one nav bounds volume can cover the house. */
	UPROPERTY(EditAnywhere, Category = "Generation", meta = (ClampMin = "1000"))
	float MaxHalfExtent = 5000.f;

	UPROPERTY(EditAnywhere, Category = "Kit")
	TArray<FPLGModuleDef> Modules;

	UPROPERTY(EditAnywhere, Category = "Kit")
	TArray<FPLGTheme> Themes;

	/** Grid the kit is checked against (cm). */
	UPROPERTY(EditAnywhere, Category = "Kit|Spec")
	float GridSize = 50.f;

	UPROPERTY(EditAnywhere, Category = "Kit|Spec")
	float WallHeight = 400.f;

	/** Full wall thickness; each module builds half of it inside its footprint. */
	UPROPERTY(EditAnywhere, Category = "Kit|Spec")
	float WallThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Kit|Spec")
	float DoorHeight = 250.f;

	/** Floor slab below Z = 0. */
	UPROPERTY(EditAnywhere, Category = "Kit|Spec")
	float FloorThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Graybox")
	FLinearColor FixedFloorColor = FLinearColor(0.55f, 0.42f, 0.32f);

	UPROPERTY(EditAnywhere, Category = "Graybox")
	FLinearColor CorridorFloorColor = FLinearColor(0.4f, 0.36f, 0.33f);

	UPROPERTY(EditAnywhere, Category = "Graybox")
	FLinearColor DefaultWallColor = FLinearColor(0.9f, 0.85f, 0.75f);

	UPROPERTY(EditAnywhere, Category = "Graybox")
	FLinearColor PropColor = FLinearColor(0.35f, 0.22f, 0.15f);

	/** Module name (and theme) floating in each module. */
	UPROPERTY(EditAnywhere, Category = "Graybox")
	bool bShowLabels = true;

	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Generation")
	FPLGLayout Layout;

private:
	UFUNCTION()
	void OnRep_Seed();

	void Generate();
	void BuildGraybox();
	void AddModuleGeometry(const FPLGPlacedModule& Placed, const FTransform& ModuleToWorld);

	/** Graybox mesh for one color, created on demand. */
	UInstancedStaticMeshComponent* GetBoxes(const FLinearColor& Color);

#if !UE_BUILD_SHIPPING
	/**
	 * -plgselftest: after the runtime navmesh settles, checks the layout, that the first player stands on the
	 * generated floor and that navigation reaches every module from the start, takes a player and a top-down
	 * screenshot, then quits. Results are [PLGSelfTest] PASS/FAIL lines in the log.
	 */
	void RunSelfTestChecks();
	void TakeTopDownShot();
	void SelfTestCheck(bool bPassed, const FString& What);
	int32 SelfTestFailures = 0;
#endif

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ShapeMaterial;

	/** One instanced mesh per graybox color; BoxColors[i] is the color of Boxes[i]. */
	UPROPERTY(Transient)
	TArray<FLinearColor> BoxColors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Boxes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextRenderComponent>> Labels;
};
