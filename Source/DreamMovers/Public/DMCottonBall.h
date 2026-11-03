#pragma once

#include "CoreMinimal.h"
#include "DMValuable.h"
#include "DMCottonBall.generated.h"

class APlayerState;
class UCameraComponent;
class USpringArmComponent;

/**
 * What is left of a caught doll. Carried like any item (light, unbreakable, worth nothing);
 * dropping it in the extraction zone revives its player there instead of banking value.
 * Its camera is the downed player's view while they wait.
 */
UCLASS()
class DREAMMOVERS_API ADMCottonBall : public ADMValuable
{
	GENERATED_BODY()

public:
	ADMCottonBall();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual FText GetDisplayName() const override;
	virtual void Extract() override;

	/** Server */
	void SetDownedPlayer(AController* InController, APlayerState* InPlayerState);
	AController* GetDownedController() const { return DownedController.Get(); }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cotton")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cotton")
	TObjectPtr<UCameraComponent> Camera;

private:
	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> DownedPlayer;

	TWeakObjectPtr<AController> DownedController;
};
