#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SwingComponent.generated.h"

UENUM(BlueprintType)
enum class ESwingState : uint8
{
    None      UMETA(DisplayName = "None"),
    Grabbing  UMETA(DisplayName = "Grabbing"),   // lerp transition to pole
    Swinging  UMETA(DisplayName = "Swinging"),   // pendulum physics active
    Releasing UMETA(DisplayName = "Releasing")
};

/**
 * Add to ASwingGameCharacter to enable pole-swinging.
 * The character swings automatically once grabbed — the player can only jump off.
 *
 * Wire-up (already done in SwingGameCharacter.cpp):
 *   DoJumpStart() → OnJumpPressed()
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SWINGGAME_API USwingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    USwingComponent();

    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // -----------------------------------------------------------------------
    // Input API
    // -----------------------------------------------------------------------

    /** Call when Jump is pressed. Grabs nearby pole (in air) or releases if swinging. */
    UFUNCTION(BlueprintCallable, Category = "Swing")
    void OnJumpPressed();

    /** @deprecated Swing input is now automatic — this method is a no-op. */
    UFUNCTION(BlueprintCallable, Category = "Swing")
    void AddSwingInput(float AxisValue);

    // -----------------------------------------------------------------------
    // State — read by AnimBP and Character
    // -----------------------------------------------------------------------

    UPROPERTY(BlueprintReadOnly, Category = "Swing|State")
    ESwingState SwingState = ESwingState::None;

    /** True whenever SwingState != None — handy for AnimBP state machine */
    UPROPERTY(BlueprintReadOnly, Category = "Swing|State")
    bool bIsSwinging = false;

    /** Pendulum angle in radians (0 = straight down, positive = forward) */
    UPROPERTY(BlueprintReadOnly, Category = "Swing|State")
    float SwingAngle = 0.0f;

    /** Angular velocity (radians / second) */
    UPROPERTY(BlueprintReadOnly, Category = "Swing|State")
    float SwingAngularVelocity = 0.0f;

    /** World-space pivot — use as Two-Bone IK effector target in AnimBP */
    UPROPERTY(BlueprintReadOnly, Category = "Swing|State")
    FVector PivotWorldLocation = FVector::ZeroVector;

    // -----------------------------------------------------------------------
    // Tuning
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing|Settings")
    float GrabDetectionRadius = 220.0f;

    /** Force applied automatically each frame to keep the swing going.
     *  Acts in the direction of current angular velocity (energy pumping). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing|Settings")
    float AutoSwingForce = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing|Settings")
    float GrabTransitionTime = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing|Settings")
    float LaunchSpeedMultiplier = 1.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing|Settings")
    float SwingDamping = 0.985f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swing|Settings")
    float MaxSwingAngle = 2.2f;

private:
    void TryGrab();
    void OnGrabbed(class ASwingPole* Pole);
    void TickGrabbing(float DeltaTime);
    void TickSwinging(float DeltaTime);
    void Release();
    FVector CalcLaunchVelocity() const;

    UPROPERTY()
    class ASwingPole* CurrentPole = nullptr;

    UPROPERTY()
    class ACharacter* OwnerCharacter = nullptr;

    float   HangLength         = 120.0f;
    float   GrabAlpha          = 0.0f;
    FVector GrabStartLocation  = FVector::ZeroVector;
    FVector GrabTargetLocation = FVector::ZeroVector;
};
