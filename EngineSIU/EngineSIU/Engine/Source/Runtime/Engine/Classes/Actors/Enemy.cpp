#include "Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ObjectFactory.h"

#include "GameFramework/Character.h"
#include "Math/Rotator.h"
#include "EnemySpawner.h"

AEnemy::AEnemy()
    : SkeletalMeshComponent(nullptr)
    , FireInterval(3.f)
    , CurrentFireTimer(0.f)
    , bShouldFire(false)
    , Character(nullptr)
{
}

UObject* AEnemy::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->FireInterval = FireInterval;
    NewActor->CurrentFireTimer = CurrentFireTimer;
    NewActor->bShouldFire = bShouldFire;
    NewActor->SkeletalMeshComponent = SkeletalMeshComponent;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentFireTimer >= 6.f)
        Destroy();

    CalculateTimer(DeltaTime);
    if (!bShouldFire) return;

    Fire();
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
    GetActorRotation();

    CurrentFireTimer = 0.0f;
}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AEnemy::Destroyed()
{
    Super::Destroyed();
    AEnemySpawner* Owner = Cast<AEnemySpawner>(GetOwner());
    Owner->SpawnedEnemy = nullptr;
}

void AEnemy::CalculateTimer(float DeltaTime)
{
    CurrentFireTimer += DeltaTime;

    if (CurrentFireTimer >= FireInterval)
    {
        //CurrentFireTimer = 0.f;

        bShouldFire = true;
    }
}

void AEnemy::Fire()
{
    // FireDirection
    // ZeroVector 대신 Character의 위치를 넣기.
    FRotator LookAtRot = FRotator::MakeLookAtRotation(this->GetActorLocation(), FVector(0, 0, 0));
    SetActorRotation(FRotator(0, LookAtRot.Yaw, 0));
}
