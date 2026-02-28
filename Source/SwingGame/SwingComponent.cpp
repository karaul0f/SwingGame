#include "SwingComponent.h"
#include "SwingPole.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
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

    // Apply slope-following rotation when walking on ground
    if (!OwnerCharacter)
        OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter && SwingState == ESwingState::None)
    {
        UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
        if (CMC && CMC->IsMovingOnGround())
        {
            TickWalkingSlope(DeltaTime);
        }
    }

    // Restore air braking once the character lands after a swing release
    if (bNeedsDecelRestore && OwnerCharacter)
    {
        UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
        if (CMC && CMC->IsMovingOnGround())
        {
            CMC->BrakingDecelerationFalling = SavedBrakingDecelFalling;
            bNeedsDecelRestore = false;
        }
    }

    bIsSwinging = (SwingState != ESwingState::None);
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

void USwingComponent::AddSwingInput(float /*AxisValue*/)
{
    // No-op — swing is now fully automatic.
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

    // Calculate grip socket local offset for rotation calculations
    if (bRotateAroundGripPoint)
    {
        if (USkeletalMeshComponent* SkelMesh = OwnerCharacter->GetMesh())
        {
            FVector GripWorldLocation = SkelMesh->GetSocketLocation(GripSocketName);
            GripSocketLocalOffset = OwnerCharacter->GetActorTransform().Inverse().TransformPosition(GripWorldLocation);
        }
        else
        {
            GripSocketLocalOffset = FVector::ZeroVector;
        }
    }

    HangLength = FMath::Max(
        FMath::Abs(PivotWorldLocation.Z - OwnerCharacter->GetActorLocation().Z),
        60.0f);

    const FVector Offset = OwnerCharacter->GetActorLocation() - PivotWorldLocation;
    SwingAngle           = FMath::Atan2(Offset.X, -Offset.Z);
    SwingAngularVelocity = 0.0f;

    UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
    if (CMC)
    {
        // Save original air braking for later restore
        if (!bNeedsDecelRestore)
        {
            SavedBrakingDecelFalling = CMC->BrakingDecelerationFalling;
        }

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

    // Auto-swing: pump energy in the direction of current motion.
    // When nearly stationary, push away from equilibrium based on angle.
    const float Dir = FMath::Abs(SwingAngularVelocity) > 0.05f
        ? FMath::Sign(SwingAngularVelocity)
        : FMath::Sign(SwingAngle);
    SwingAngularVelocity += Dir * AutoSwingForce * DeltaTime;

    SwingAngularVelocity *= FMath::Pow(SwingDamping, DeltaTime * 60.0f);

    SwingAngle = FMath::Clamp(
        SwingAngle + SwingAngularVelocity * DeltaTime,
        -MaxSwingAngle, MaxSwingAngle);

    const FVector OldPos = OwnerCharacter->GetActorLocation();

    // Calculate target rotation for facing direction
    FRotator TargetRotation = OwnerCharacter->GetActorRotation();
    const float     TSign  = (SwingAngularVelocity >= 0.0f) ? 1.0f : -1.0f;
    const FVector   FaceDir = FVector(FMath::Cos(SwingAngle) * TSign, 0.0f, 0.0f);
    if (!FaceDir.IsNearlyZero())
    {
        TargetRotation = FaceDir.Rotation();
    }

    // If rotating around grip point, hang from paw with body below beam
    if (bRotateAroundGripPoint && !GripSocketLocalOffset.IsNearlyZero())
    {
        // Base 90° pitch so paw points up (toward beam) and body hangs down
        // Swing angle adds lean on top of that
        float LeanAmount = FMath::RadiansToDegrees(SwingAngle) * TSign * 0.8f;
        TargetRotation.Pitch = 90.0f + LeanAmount;
    }

    // Calculate swing position
    FVector NewPos;
    if (bRotateAroundGripPoint && !GripSocketLocalOffset.IsNearlyZero())
    {
        // Pin grip socket to beam, body hangs below via rotation
        FVector RotatedGripOffset = TargetRotation.RotateVector(GripSocketLocalOffset);
        NewPos = PivotWorldLocation - RotatedGripOffset;
    }
    else
    {
        // Default: character center moves in arc below pivot
        NewPos = PivotWorldLocation
            + FVector(FMath::Sin(SwingAngle), 0.0f, -FMath::Cos(SwingAngle)) * HangLength;
    }

    if (DeltaTime > KINDA_SMALL_NUMBER)
    {
        CachedSwingVelocity = (NewPos - OldPos) / DeltaTime;
    }

    OwnerCharacter->SetActorLocation(NewPos, false, nullptr, ETeleportType::TeleportPhysics);
    OwnerCharacter->SetActorRotation(
        FMath::RInterpTo(OwnerCharacter->GetActorRotation(), TargetRotation, DeltaTime, 12.0f));
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

        // Remove air braking so horizontal momentum is preserved in flight
        CMC->BrakingDecelerationFalling = 0.0f;
        bNeedsDecelRestore = true;
    }

    OwnerCharacter->LaunchCharacter(CalcLaunchVelocity(), true, true);
    CurrentPole = nullptr;
}

// ---------------------------------------------------------------------------
// Walking slope alignment
// ---------------------------------------------------------------------------

void USwingComponent::TickWalkingSlope(float DeltaTime)
{
    if (!OwnerCharacter) return;

    UCharacterMovementComponent* CMC = OwnerCharacter->GetCharacterMovement();
    if (!CMC) return;

    // Get character feet location for trace
    FVector TraceStart = OwnerCharacter->GetActorLocation();
    FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 200.0f);

    // Trace to find ground surface
    FHitResult HitResult;
    FCollisionQueryParams QueryParams(FName(TEXT("SlopeTrace")), false, OwnerCharacter);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_WorldStatic,
        QueryParams);

    if (!bHit || !HitResult.IsValidBlockingHit())
    {
        // No ground found, return to upright rotation
        FRotator CurrentRot = OwnerCharacter->GetActorRotation();
        FRotator TargetRot(0.0f, CurrentRot.Yaw, 0.0f);
        OwnerCharacter->SetActorRotation(
            FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 5.0f));
        return;
    }

    // Get surface normal
    FVector SurfaceNormal = HitResult.ImpactNormal;

    // Calculate pitch and roll from surface normal
    // Pitch: tilt forward/backward based on X component of normal
    // Roll: tilt side-to-side based on Y component of normal
    float PitchAngle = FMath::Atan2(-SurfaceNormal.X, SurfaceNormal.Z);
    float RollAngle = FMath::Atan2(SurfaceNormal.Y, SurfaceNormal.Z);

    // Factor in movement direction for dynamic lean
    FVector Velocity = CMC->Velocity;
    FVector HorizontalVelocity = FVector(Velocity.X, Velocity.Y, 0.0f);

    if (!HorizontalVelocity.IsNearlyZero())
    {
        // Normalize movement direction
        FVector MovementDir = HorizontalVelocity.GetSafeNormal();

        // Get the character's forward direction (affected only by yaw, not pitch/roll)
        FRotator CurrentRot = OwnerCharacter->GetActorRotation();
        FVector CharForward = FRotationMatrix(FRotator(0.0f, CurrentRot.Yaw, 0.0f)).GetUnitAxis(EAxis::X);

        // Calculate how much the movement direction aligns with surface normal
        // This determines if character is moving up or down slope
        float SlopeInfluence = FVector::DotProduct(MovementDir, SurfaceNormal);

        // Add movement-based lean: character leans forward when moving upslope
        // Clamp the lean to prevent over-tilting (max ±30 degrees additional lean)
        float MovementLean = FMath::Clamp(SlopeInfluence * 45.0f, -30.0f, 30.0f);
        PitchAngle += FMath::DegreesToRadians(MovementLean);
    }

    // Create target rotation (preserve character's yaw)
    FRotator CurrentRot = OwnerCharacter->GetActorRotation();
    FRotator TargetRot(
        FMath::RadiansToDegrees(PitchAngle),
        CurrentRot.Yaw,
        FMath::RadiansToDegrees(RollAngle));

    // Smoothly interpolate to target rotation
    FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, DeltaTime, 5.0f);
    OwnerCharacter->SetActorRotation(NewRot);
}

FVector USwingComponent::CalcLaunchVelocity() const
{
    return CachedSwingVelocity * LaunchSpeedMultiplier;
}
