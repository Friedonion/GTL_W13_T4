#include "Enemy.h"
#include "Components/SkeletalMeshComponent.h"
#include "Classes/Engine/SkeletalMesh.h"
#include "UObject/ObjectFactory.h"

#include "GameFramework/Character.h"
#include "Math/Rotator.h"
#include "EnemySpawner.h"

#include "Actors/Bullet.h"
#include "Engine/EditorEngine.h"

AEnemy::AEnemy()
    : SkeletalMeshComponent(nullptr)
    , SkeletalMesh(nullptr)
    , FireInterval(4.f)
    , CurrentFireTimer(0.f)
    , bShouldFire(false)
    , Character(nullptr)
    , Bullet(nullptr)
{
}

UObject* AEnemy::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));
    NewActor->FireInterval = FireInterval;
    NewActor->CurrentFireTimer = CurrentFireTimer;
    NewActor->bShouldFire = bShouldFire;
    NewActor->SkeletalMeshComponent = SkeletalMeshComponent;
    NewActor->SkeletalMesh = SkeletalMesh;
    NewActor->Bullet = Bullet;

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    // FireDirection
    // ZeroVector 대신 Character의 위치를 넣기.
    FRotator LookAtRot = FRotator::MakeLookAtRotation(this->GetActorLocation(), FVector(0, 0, 0));
    SetActorRotation(FRotator(0, LookAtRot.Yaw, 0));

    // Destroy로직은 다른 곳에 추가할 예정
    if (CurrentFireTimer >= 5.f)
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

    SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName("Contents/Enemy/Pistol_Idle"));
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
    if (Bullet != nullptr) return;

    UWorld* World = GEngine->ActiveWorld;

    Bullet = World->SpawnActor<ABullet>();
    //Bullet->SetActorLabel(TEXT("OBJ_BULLET"));
    Bullet->SetOwner(this);
    //Bullet

}
