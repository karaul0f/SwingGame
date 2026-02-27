#include "SwingPole.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

ASwingPole::ASwingPole()
{
    PrimaryActorTick.bCanEverTick = false;

    GrabTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("GrabTrigger"));
    SetRootComponent(GrabTrigger);
    GrabTrigger->SetSphereRadius(200.0f);
    GrabTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    GrabTrigger->SetGenerateOverlapEvents(true);

    PoleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PoleMesh"));
    PoleMesh->SetupAttachment(GrabTrigger);
    PoleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FVector ASwingPole::GetGrabPoint(const FVector& CharacterLocation) const
{
    const FVector PoleDir   = GetActorRightVector();
    const FVector PoleStart = GetActorLocation() - PoleDir * PoleHalfLength;

    const float T = FMath::Clamp(
        FVector::DotProduct(CharacterLocation - PoleStart, PoleDir),
        0.0f,
        PoleHalfLength * 2.0f
    );

    const FVector Closest = PoleStart + PoleDir * T;
    // Keep pole's Z — hands always at pole height
    return FVector(Closest.X, Closest.Y, GetActorLocation().Z);
}
