#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DMValuable.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * A fragile antique the players carry to the extraction zone.
 * Physics and value live on the server; clients only see the replicated result.
 * Every hard impact cracks it and costs value, and the last crack breaks it.
 */
UCLASS()
class DREAMMOVERS_API ADMValuable : public AActor
{
	GENERATED_BODY()

public:
	ADMValuable();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UStaticMeshComponent* GetMesh() const { return Mesh; }
	virtual FText GetDisplayName() const { return DisplayName; }
	int32 GetCurrentValue() const { return CurrentValue; }
	int32 GetCracks() const { return Cracks; }
	int32 GetMaxCracks() const { return MaxCracks; }
	int32 GetNumHolders() const { return NumHolders; }
	float GetMassKg() const { return MassKg; }
	bool IsExtracted() const { return bExtracted; }

	/** Server: called by grab components as players start and stop holding this. */
	void AddHolder();
	void RemoveHolder();

	/** Server: bank the current value and remove the item. */
	virtual void Extract();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Valuable")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "0"))
	int32 BaseValue = 1000;

	/** Heavier than one player's grab strength (see UDMGrabComponent::MaxLiftKg) means it needs two. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "0.1"))
	float MassKg = 2.f;

	/** Impact speed (cm/s) along the contact normal that starts cracking it. Lower = more fragile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "50"))
	float CrackSpeed = 350.f;

	/** Fraction of BaseValue lost by a crack right at CrackSpeed; harder hits lose more. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "0", ClampMax = "1"))
	float LossPerCrack = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "1"))
	int32 MaxCracks = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable")
	FLinearColor Tint = FLinearColor(0.9f, 0.75f, 0.45f);

	/** Glass and polished brass are glossy (low roughness), which is what the oil pastel shader turns into star glints. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valuable", meta = (ClampMin = "0", ClampMax = "1"))
	float Metallic = 0.f;

	/** Bumps slower than this (cm/s) are silent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float QuietSpeed = 150.f;

	/** How far a bump carries: from a soft knock at QuietSpeed up to a clatter at 800 cm/s. Cracks and breaks are louder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	FVector2D BumpNoiseRadius = FVector2D(300.f, 1500.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
	float BreakNoiseRadius = 2500.f;

private:
	UFUNCTION()
	void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void Crack(float ImpactSpeed);

	UFUNCTION()
	void OnRep_Cracks();

	void ApplyMassAndColor();

protected:
	/** For subclasses that leave the map through the extraction zone without banking value. */
	void MarkExtracted() { bExtracted = true; }

private:

	UPROPERTY(Replicated)
	int32 CurrentValue = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Cracks)
	int32 Cracks = 0;

	UPROPERTY(Replicated)
	int32 NumHolders = 0;

	bool bExtracted = false;

	/** Velocity from the previous tick: the hit callback fires after the collision already changed it. */
	FVector LastVelocity = FVector::ZeroVector;
	double LastCrackTime = -1.0;
	double LastNoiseTime = -1.0;
	double HitsEnabledTime = 0.0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TintMaterial;
};
