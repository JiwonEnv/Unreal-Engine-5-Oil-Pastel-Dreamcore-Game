#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DMMonster.generated.h"

class ADMCharacter;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UENUM()
enum class EDMMonsterState : uint8
{
	Idle,
	Patrol,
	/** Saw a doll and is going after it. */
	Chase,
	/** Rushing toward a noise. */
	Charge,
	/** At the last known spot, looking around. */
	Search,
	/** Heading back to its patrol or post. */
	Return
};

/**
 * Base for the living antiques. Brains run on the server only (a small state machine in ServerThink,
 * moving with the default AI controller over the runtime navmesh); clients just see the replicated
 * movement and state. Touching a doll downs it.
 */
UCLASS(Abstract)
class DREAMMOVERS_API ADMMonster : public ACharacter
{
	GENERATED_BODY()

public:
	ADMMonster();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	EDMMonsterState GetMonsterState() const { return MonsterState; }
	const FText& GetDisplayName() const { return DisplayName; }

protected:
	virtual void BeginPlay() override;

	/** Server only, every tick. */
	virtual void ServerThink(float DeltaSeconds) {}

	/** Server and clients, whenever the state changes. */
	virtual void OnMonsterStateChanged() {}

	void SetMonsterState(EDMMonsterState NewState);
	double StateAge() const;

	void MoveToLocation(const FVector& Goal, float Speed);
	void MoveToActor(AActor* Goal, float Speed);
	void StopMoving();
	/** True once the current move finished or failed. */
	bool IsMoveDone() const;

	/** Downs the first doll within CatchRadius. */
	ADMCharacter* TryCatch();

	/** Placeholder body part, colored at BeginPlay. Call from the constructor. */
	UStaticMeshComponent* AddPart(FName Name, UStaticMesh* Shape, const FVector& Location, const FVector& Scale,
		const FLinearColor& Color, float PartRoughness = 0.6f, float PartMetallic = 0.f, const FRotator& Rotation = FRotator::ZeroRotator);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster")
	FText DisplayName;

	/** Extra reach beyond the two capsules touching. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster")
	float CatchRadius = 40.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeShape;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CylinderShape;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> SphereShape;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> ConeShape;

private:
	UFUNCTION()
	void OnRep_MonsterState();

	UPROPERTY(ReplicatedUsing = OnRep_MonsterState)
	EDMMonsterState MonsterState = EDMMonsterState::Idle;

	double StateStartTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> PartMaterial;

	// Filled by the constructor of every instance (the parts are default subobjects, owned by the actor),
	// so these are deliberately not UPROPERTYs that archetype copying would overwrite.
	TArray<UStaticMeshComponent*> Parts;

	/** Per part: color, then (roughness, metallic). */
	TArray<FLinearColor> PartColors;
	TArray<FVector2D> PartGloss;
};
