#include "SwingAnimInstance.h"
#include "SwingComponent.h"
#include "GameFramework/Character.h"

void USwingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!SwingComp)
    {
        if (ACharacter* Owner = Cast<ACharacter>(TryGetPawnOwner()))
            SwingComp = Owner->FindComponentByClass<USwingComponent>();
    }
    if (!SwingComp) return;

    SwingState           = SwingComp->SwingState;
    SwingAngle           = SwingComp->SwingAngle;
    SwingAngularVelocity = SwingComp->SwingAngularVelocity;
    PivotLocation        = SwingComp->PivotWorldLocation;
    bIsSwinging          = SwingComp->bIsSwinging;

    // Smooth IK weight: fast on, slower off
    const float TargetAlpha = bIsSwinging ? 1.0f : 0.0f;
    const float Speed       = (TargetAlpha > IKAlpha) ? 8.0f : 5.0f;
    IKAlpha = FMath::FInterpTo(IKAlpha, TargetAlpha, DeltaSeconds, Speed);
}
