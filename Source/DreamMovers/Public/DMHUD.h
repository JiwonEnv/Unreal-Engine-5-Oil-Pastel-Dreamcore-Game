#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DMHUD.generated.h"

class UFont;
class UMaterialInstanceDynamic;

/** Prototype HUD drawn straight to the canvas: score, timer, crosshair and price tags. */
UCLASS()
class DREAMMOVERS_API ADMHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

protected:
	/** Price tags are drawn on items closer than this. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float TagDistance = 700.f;

	/** The bell horse's jingle is "heard" (shown) within this distance while it moves. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	float JingleDistance = 2500.f;

	/** Dream stability left when the picture starts losing its color. */
	UPROPERTY(EditDefaultsOnly, Category = "Dream", meta = (ClampMin = "0", ClampMax = "1"))
	float CollapseStartsAt = 0.5f;

	/** How far the picture fades toward a bare sketch when stability reaches zero (1 = sketch only). */
	UPROPERTY(EditDefaultsOnly, Category = "Dream", meta = (ClampMin = "0", ClampMax = "1"))
	float CollapseAtWake = 0.75f;

private:
	void DrawRoundInfo();
	void DrawPriceTags();
	void DrawCrosshairAndTarget();
	void DrawMonsterCues();
	void DrawDownedMessage();
	void DrawHidingPrompt();
	void DrawCenteredText(const FText& Text, FVector2D Position, const FLinearColor& Color, int32 Size);

	/** Swaps the post-process volumes' oil pastel material for dynamic instances this HUD can drive. */
	void BindDreamMaterials();
	void UpdateDreamCollapse(float DeltaSeconds);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DreamMaterials;

	float Collapse = 0.f;

	/** Runtime-cached copy of the Slate default font, which falls back to a CJK font for Korean. */
	UPROPERTY(Transient)
	TObjectPtr<UFont> HudFont;
};
