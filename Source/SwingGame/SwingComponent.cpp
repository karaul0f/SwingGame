#include "SwingComponent.h"
#include "SwingPole.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "CollisionShape.h"
#include "WorldCollision.h"
#include "CollisionQueryParams.h"

USwingComponent::USwingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void USwingComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    switch (SwingState)
    {
        case ESwingState::Grabbing: TickGrabbing(DeltaTime); break;
        case ESwingState::Swinging: TickSwinging(DeltaTime); break;
        default: break;
    }

    bIsSwinging   = (SwingState != ESwingState::None);
    PendulumInput = 0.0f;
}

// ---------------------------------------------------------------------------
// Input API
// ---------------------------------------------------------------------------

void USwingComponent::OnJumpPressed()
{
    if (SwingState == ESwingState::None)
        TryGrab();
    else if (SwingState == ESwingState::Swinging)
        Release();
}

void USwingComponent::AddSwingInput(float AxisValue)
{
    PendulumInput = AxisValue;
}

// ---------------------------------------------------------------------------
// Grab detection
// ---------------------------------------------------------------------------

void USwingComponent::TryGrab()
{
    if (!OwnerCharacter)
        OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
    if (CMC && CMC->IsMovingOnGround()) return;

    const FVector Origin = OwnerCharacter->GetActorLocation();

    TArray<FOverlapResult> Overlaps;
    FCollisionShape        Sphere = FCollisionShape::MakeSphere(GrabDetectionRadius);
    FCollisionQueryParams  Params(FName(TEXT("SwingGrab")), false, OwnerCharacter);

    GetWorld()->OverlapMultiByChannel(
        Overlaps, Origin, FQuat::Identity,
        ECC_WorldDynamic, Sphere, Params);

    ASwingPole* BestPole = nullptr;
    float       BestDist = TNumericLimits<float>::Max();

    for (const FOverlapResult& Result : Overlaps)
    {
        ASwingPole* Pole = Cast<ASwingPole>(Result.GetActor());
        if (!Pole) continue;
        if (Pole->GetActorLocation().Z <= Origin.Z) continue;  // must be above

        const float Dist = FVector::Dist(Origin, Pole->GetActorLocation());
        if (Dist < BestDist) { BestDist = Dist; BestPole = Pole; }
    }

    if (BestPole) OnGrabbed(BestPole);
}

// ---------------------------------------------------------------------------
// Start grab
// ---------------------------------------------------------------------------

void USwingComponent::OnGrabbed(ASwingPole* Pole)
{
    if (!OwnerCharacter)
        OwnerCharacter = Cast<ACharacter>(GetOwner());

    CurrentPole        = Pole;
    PivotWorldLocation = Pole->GetGrabPoint(OwnerCharacter->GetActorLocation());

    HangLength = FMath::Max(
        FMath::Abs(PivotWorldLocation.Z - OwnerCharacter->GetActorLocation().Z),
        60.0f);

    const FVector Offset = OwnerCharacter->GetActorLocation() - PivotWorldLocation;
    SwingAngle           = FMath::Atan2(Offset.X, -Offset.Z);
    SwingAngularVelocity = 0.0f;

    UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
    if (CMC)
    {
        CMC->StopMovementImmediately();
        CMC->GravityScale = 0.0f;
        CMC->SetMovementMode(MOVE_Flying);
    }

    GrabAlpha          = 0.0f;
    GrabStartLocation  = OwnerCharacter->GetActorLocation();
    GrabTargetLocation = PivotWorldLocation
        + FVector(FMath::Sin(SwingAngle), 0.0f, -FMath::Cos(SwingAngle)) * HangLength;

    SwingState = ESwingState::Grabbing;
}

// ---------------------------------------------------------------------------
// Grabbing tick — smooth lerp to hang position
// ---------------------------------------------------------------------------

void USwingComponent::TickGrabbing(float DeltaTime)
{
    GrabAlpha += DeltaTime / FMath::Max(GrabTransitionTime, KINDA_SMALL_NUMBER);
    if (GrabAlpha >= 1.0f)
    {
        GrabAlpha  = 1.0f;
        SwingState = ESwingState::Swinging;
    }

    const FVector NewPos = FMath::Lerp(
        GrabStartLocation,
        GrabTargetLocation,
        FMath::SmoothStep(0.0f, 1.0f, GrabAlpha));

    OwnerCharacter->SetActorLocation(NewPos, false, nullptr, ETeleportType::TeleportPhysics);
}

// ---------------------------------------------------------------------------
// Swinging tick — pendulum physics
// ---------------------------------------------------------------------------

void USwingComponent::TickSwinging(float DeltaTime)
{
    // α = -(g / L) * sin(θ)
    constexpr float G     = 980.0f;
    const float     Alpha = -(G / HangLength) * FMath::Sin(SwingAngle);

    SwingAngularVelocity += Alpha * DeltaTime;
    SwingAngularVelocity += PendulumInput * SwingInputBoost * DeltaTime;
    SwingAngularVelocity *= FMath::Pow(SwingDamping, DeltaTime * 60.0f);

    SwingAngle = FMath::Clamp(
        SwingAngle + SwingAngularVelocity * DeltaTime,
        -MaxSwingAngle, MaxSwingAngle);

    const FVector NewPos = PivotWorldLocation
        + FVector(FMath::Sin(SwingAngle), 0.0f, -FMath::Cos(SwingAngle)) * HangLength;

    OwnerCharacter->SetActorLocation(NewPos, false, nullptr, ETeleportType::TeleportPhysics);

    // Rotate character to face swing direction
    const float     TSign  = (SwingAngularVelocity >= 0.0f) ? 1.0f : -1.0f;
    const FVector   FaceDir = FVector(FMath::Cos(SwingAngle) * TSign, 0.0f, 0.0f);
    if (!FaceDir.IsNearlyZero())
    {
        const FRotator Target = FaceDir.Rotation();
        OwnerCharacter->SetActorRotation(
            FMath::RInterpTo(OwnerCharacter->GetActorRotation(), Target, DeltaTime, 12.0f));
    }
}

// ---------------------------------------------------------------------------
// Release
// ---------------------------------------------------------------------------

void USwingComponent::Release()
{
    SwingState = ESwingState::None;

    UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
    if (CMC)
    {
        CMC->GravityScale = 1.0f;
        CMC->SetMovementMode(MOVE_Falling);
    }

    OwnerCharacter->LaunchCharacter(CalcLaunchVelocity(), true, true);
    CurrentPole = nullptr;
}

FVector USwingComponent::CalcLaunchVelocity() const
{
    const float Speed      = SwingAngularVelocity * HangLength * LaunchSpeedMultiplier;
    const float TSign      = (SwingAngularVelocity >= 0.0f) ? 1.0f : -1.0f;
    const float TangAngle  = SwingAngle + TSign * HALF_PI;
    return FVector(FMath::Sin(TangAngle), 0.0f, -FMath::Cos(TangAngle)) * Speed;
}
