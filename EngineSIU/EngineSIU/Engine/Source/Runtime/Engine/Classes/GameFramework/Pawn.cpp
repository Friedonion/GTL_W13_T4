#include "Pawn.h"

void APawn::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();
}

UObject* APawn::Duplicate(UObject* InOuter)
{
    return Super::Duplicate(InOuter);
}

void APawn::BeginPlay()
{
    Super::BeginPlay();
}

void APawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void APawn::Destroyed()
{
    Super::Destroyed();
}

void APawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}
