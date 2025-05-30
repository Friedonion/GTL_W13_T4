#include "Enemy.h"
#include "Components/SkeletalMeshComponent.h"


UObject* AEnemy::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->SkeletalMeshComponent = SkeletalMeshComponent; // FIX-ME

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AEnemy::Destroyed()
{
}

void AEnemy::Fire()
{
}
