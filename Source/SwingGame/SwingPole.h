#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SwingPole.generated.h"

/**
 * A horizontal pole the character can grab and swing on.
 * Create a Blueprint child (BP_SwingPole), assign a cylinder mesh,
 * and place it ~250 cm above the floor.
 */
UCLASS()
class SWINGGAME_API ASwingPole : public AActor
{
    GENERATED_BODY()

public:
    ASwingPole();

    /**
     * Returns the closest point on the pole axis to CharacterLocation.
     * Z is locked to the pole's own height so hands always hang at the right level.
     */
    UFUNCTION(BlueprintCallable, Category = "Swing")
    FVector GetGrabPoint(const FVector& CharacterLocation) const;

    /** Visual mesh — assign a horizontal cylinder in the Blueprint */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* PoleMesh;

    /** Trigger sphere used by SwingComponent to detect this pole */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* GrabTrigger;

    /** Half-length of the pole along its local right vector (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pole")
    float PoleHalfLength = 150.0f;
};
