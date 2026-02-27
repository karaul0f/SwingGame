#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "SwingComponent.h"
#include "SwingAnimInstance.generated.h"

/**
 * AnimInstance for the swinging character.
 * Assign this (or a Blueprint child) as the Anim Class on the Skeletal Mesh.
 *
 * In AnimBP:
 *  - bIsSwinging  → drives state machine (Locomotion ↔ SwingLoop)
 *  - SwingAngle   → BlendSpace 1D input for SwingLoop
 *  - PivotLocation → Two-Bone IK effector target for both hands
 *  - IKAlpha      → Two-Bone IK blend weight
 */
UCLASS()
class SWINGGAME_API USwingAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category = "Swing")
    bool bIsSwinging = false;

    UPROPERTY(BlueprintReadOnly, Category = "Swing")
    ESwingState SwingState = ESwingState::None;

    /** Radians, range ≈ -2.2 … 2.2. Feed into a BlendSpace 1D for SwingLoop. */
    UPROPERTY(BlueprintReadOnly, Category = "Swing")
    float SwingAngle = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Swing")
    float SwingAngularVelocity = 0.0f;

    /** World-space pivot — set as Two-Bone IK effector for both hand bones. */
    UPROPERTY(BlueprintReadOnly, Category = "Swing")
    FVector PivotLocation = FVector::ZeroVector;

    /** IK blend weight: ramps 0→1 on grab, 1→0 on release. */
    UPROPERTY(BlueprintReadOnly, Category = "Swing")
    float IKAlpha = 0.0f;

private:
    UPROPERTY()
    class USwingComponent* SwingComp = nullptr;
};
