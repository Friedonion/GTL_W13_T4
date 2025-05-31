#include "Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ObjectFactory.h"

AEnemy::AEnemy()
    : SkeletalMeshComponent(nullptr)
    , FireInterval(3.f)
{
}

UObject* AEnemy::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->FireInterval = FireInterval;

    //NewActor->SkeletalMeshComponent = SkeletalMeshComponent;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AEnemy::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(ELogLevel::Display, TEXT("AEnemy has been spawned."));

    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>();
    SetRootComponent(SkeletalMeshComponent);
    USkeletalMesh* SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName("Contents/Enemy/Enemy_T-Pose"));
    SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    //SetActorRotation();
}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AEnemy::Destroyed()
{
    Super::Destroyed();

}

void AEnemy::Fire()
{
}
