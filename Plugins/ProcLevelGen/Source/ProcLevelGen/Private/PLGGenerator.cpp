#include "PLGGenerator.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "PLGLayout.h"
#include "ProcLevelGen.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Marks everything the generator builds, so a rebuild can sweep it up even after PIE duplication. */
	const FName GeneratedTag(TEXT("PLGGenerated"));
}

APLGGenerator::APLGGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Shape(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	CubeMesh = Cube.Object;
	ShapeMaterial = Shape.Object;

	Modules = PLG::MakeSampleModules();
	Themes = PLG::MakeSampleThemes();
}

void APLGGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APLGGenerator, Seed);
}

void APLGGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (bNewSeedEachRun || Seed == 0)
		{
			Seed = FMath::RandRange(1, MAX_int32 - 1);
		}
		Generate();
	}
	else if (!bNewSeedEachRun)
	{
		// The saved seed is the server's seed too, so OnRep may never fire.
		Generate();
	}
}

void APLGGenerator::OnRep_Seed()
{
	Generate();
}

void APLGGenerator::Preview()
{
	if (Seed == 0)
	{
		Seed = 1;
	}
	Generate();
}

void APLGGenerator::PreviewNewSeed()
{
	Seed = FMath::RandRange(1, MAX_int32 - 1);
	Generate();
}

void APLGGenerator::ClearPreview()
{
	Layout = FPLGLayout();
	BuildGraybox();
}

FTransform APLGGenerator::GetModuleTransform(int32 PlacedIndex) const
{
	if (!Layout.Modules.IsValidIndex(PlacedIndex))
	{
		return GetActorTransform();
	}
	const FPLGPlacedModule& Placed = Layout.Modules[PlacedIndex];
	const FTransform Local(FRotator(0.f, 90.f * Placed.Quarter, 0.f), FVector(Placed.Location, 0.f));
	return Local * GetActorTransform();
}

void APLGGenerator::Generate()
{
	for (const FString& Problem : PLG::ValidateModules(Modules, GridSize))
	{
		UE_LOG(LogProcLevelGen, Warning, TEXT("%s: kit: %s"), *GetName(), *Problem);
	}

	FPLGLayoutSettings Settings;
	Settings.Modules = Modules;
	Settings.NumThemes = Themes.Num();
	Settings.RoomCount = RoomCount;
	Settings.CorridorChance = CorridorChance;
	Settings.MaxHalfExtent = MaxHalfExtent;

	FString Report;
	if (PLG::GenerateLayout(Settings, Seed, Layout, Report))
	{
		UE_LOG(LogProcLevelGen, Log, TEXT("%s: %s"), *GetName(), *Report);
		for (int32 i = 0; i < Layout.Modules.Num(); ++i)
		{
			const FPLGPlacedModule& Placed = Layout.Modules[i];
			if (!Placed.bRemoved)
			{
				UE_LOG(LogProcLevelGen, Verbose, TEXT("  [%d] %s at (%.0f, %.0f) yaw %d depth %d theme %d"), i, *Modules[Placed.Def].Name.ToString(),
					Placed.Location.X, Placed.Location.Y, Placed.Quarter * 90, Placed.Depth, Placed.Theme);
			}
		}
	}
	else
	{
		UE_LOG(LogProcLevelGen, Error, TEXT("%s: %s"), *GetName(), *Report);
	}

	BuildGraybox();
	OnGenerated.Broadcast(this);
}

UInstancedStaticMeshComponent* APLGGenerator::GetBoxes(const FLinearColor& Color)
{
	for (int32 i = 0; i < BoxColors.Num(); ++i)
	{
		if (BoxColors[i].Equals(Color) && Boxes[i])
		{
			return Boxes[i];
		}
	}
	UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
	Instances->ComponentTags.Add(GeneratedTag);
	Instances->SetMobility(EComponentMobility::Movable);
	Instances->SetStaticMesh(CubeMesh);
	Instances->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(ShapeMaterial, Instances))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Instances->SetMaterial(0, Material);
	}
	Instances->SetupAttachment(GetRootComponent());
	Instances->RegisterComponent();
	BoxColors.Add(Color);
	Boxes.Add(Instances);
	return Instances;
}

void APLGGenerator::BuildGraybox()
{
	TInlineComponentArray<UActorComponent*> Existing(this);
	for (UActorComponent* Component : Existing)
	{
		if (Component && Component->ComponentHasTag(GeneratedTag))
		{
			Component->DestroyComponent();
		}
	}
	BoxColors.Reset();
	Boxes.Reset();
	Labels.Reset();

	for (int32 i = 0; i < Layout.Modules.Num(); ++i)
	{
		const FPLGPlacedModule& Placed = Layout.Modules[i];
		if (Placed.bRemoved || !Modules.IsValidIndex(Placed.Def))
		{
			continue;
		}
		const FTransform ModuleToWorld = GetModuleTransform(i);
		AddModuleGeometry(Placed, ModuleToWorld);

		if (bShowLabels)
		{
			FString Text = Modules[Placed.Def].Name.ToString();
			if (Themes.IsValidIndex(Placed.Theme))
			{
				Text += TEXT("\n") + Themes[Placed.Theme].Name.ToString();
			}
			UTextRenderComponent* Label = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
			Label->ComponentTags.Add(GeneratedTag);
			Label->SetMobility(EComponentMobility::Movable);
			Label->SetText(FText::FromString(Text));
			Label->SetHorizontalAlignment(EHTA_Center);
			Label->SetVerticalAlignment(EVRTA_TextCenter);
			Label->SetWorldSize(80.f);
			Label->SetTextRenderColor(FColor::Black);
			// Readable from the editor's top view; players never see it.
			Label->SetHiddenInGame(true);
			Label->SetupAttachment(GetRootComponent());
			Label->SetWorldLocationAndRotation(ModuleToWorld.TransformPosition(FVector(0.f, 0.f, 10.f)), FRotator(90.f, ModuleToWorld.Rotator().Yaw, 0.f));
			Label->RegisterComponent();
			Labels.Add(Label);
		}
	}
}

void APLGGenerator::AddModuleGeometry(const FPLGPlacedModule& Placed, const FTransform& ModuleToWorld)
{
	const FPLGModuleDef& Def = Modules[Placed.Def];

	FLinearColor FloorColor = FixedFloorColor;
	FLinearColor WallColor = DefaultWallColor;
	if (Def.Role == EPLGRole::Corridor)
	{
		FloorColor = CorridorFloorColor;
	}
	else if (Def.Role == EPLGRole::Room && Themes.IsValidIndex(Placed.Theme))
	{
		FloorColor = Themes[Placed.Theme].FloorColor;
		WallColor = Themes[Placed.Theme].WallColor;
	}
	UInstancedStaticMeshComponent* Floor = GetBoxes(FloorColor);
	UInstancedStaticMeshComponent* Walls = GetBoxes(WallColor);

	// Engine cube: 100 cm, centered. Boxes are given in module space (pivot = floor top, footprint center).
	auto AddBox = [&ModuleToWorld](UInstancedStaticMeshComponent* Instances, const FVector& Center, const FVector& Size)
	{
		Instances->AddInstance(FTransform(FQuat::Identity, Center, Size / 100.f) * ModuleToWorld, /*bWorldSpace*/ true);
	};

	AddBox(Floor, FVector(0.f, 0.f, -FloorThickness * 0.5f), FVector(Def.Size.X, Def.Size.Y, FloorThickness));

	// Half the wall thickness, inside the footprint: the neighbor builds the other half.
	const float Half = WallThickness * 0.5f;
	for (int32 EdgeIndex = 0; EdgeIndex < 4; ++EdgeIndex)
	{
		const EPLGEdge Edge = EPLGEdge(EdgeIndex);
		const bool bXEdge = Edge == EPLGEdge::PosX || Edge == EPLGEdge::NegX;
		const float Sign = (Edge == EPLGEdge::PosX || Edge == EPLGEdge::PosY) ? 1.f : -1.f;
		const float Length = bXEdge ? Def.Size.Y : Def.Size.X;
		const float Across = Sign * ((bXEdge ? Def.Size.X : Def.Size.Y) * 0.5f - Half * 0.5f);
		// Walls on the Y edges stop where the X edge walls already fill the corners.
		const float Trim = bXEdge ? 0.f : Half;

		TArray<FVector2D> Gaps;
		for (int32 S = 0; S < Def.Sockets.Num(); ++S)
		{
			const FPLGSocket& Socket = Def.Sockets[S];
			if (Socket.Edge == Edge && Placed.Links.IsValidIndex(S) && Placed.Links[S] != INDEX_NONE)
			{
				Gaps.Emplace(Socket.Offset - Socket.Width * 0.5f, Socket.Offset + Socket.Width * 0.5f);
			}
		}
		Gaps.Sort([](const FVector2D& A, const FVector2D& B) { return A.X < B.X; });

		auto Segment = [&](float From, float To, float Bottom, float Top)
		{
			if (To - From < 0.5f || Top - Bottom < 0.5f)
			{
				return;
			}
			const float Along = (From + To) * 0.5f;
			const float Z = (Bottom + Top) * 0.5f;
			const FVector Center = bXEdge ? FVector(Across, Along, Z) : FVector(Along, Across, Z);
			const FVector Size = bXEdge ? FVector(Half, To - From, Top - Bottom) : FVector(To - From, Half, Top - Bottom);
			AddBox(Walls, Center, Size);
		};

		float Cursor = -Length * 0.5f + Trim;
		for (const FVector2D& Gap : Gaps)
		{
			Segment(Cursor, Gap.X, 0.f, WallHeight);
			Segment(Gap.X, Gap.Y, DoorHeight, WallHeight); // lintel
			Cursor = Gap.Y;
		}
		Segment(Cursor, Length * 0.5f - Trim, 0.f, WallHeight);
	}
}
