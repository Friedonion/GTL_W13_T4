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
    , Direction(FRotator::ZeroRotator)
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

    return NewActor;
}

void AEnemy::Tick(float DeltaTime)
{
    FVector PlayerLocation = GEngine->ActiveWorld->GetMainPlayer()->GetActorLocation();
    Direction = FRotator::MakeLookAtRotation(this->GetActorLocation(), PlayerLocation);
    
    SetActorRotation(FRotator(0, Direction.Yaw, 0));

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

    SkeletalMeshComponent->ClearAnimScriptInstance();

    SkeletalMesh = UAssetManager::Get().GetSkeletalMesh(FName("Contents/Enemy/Pistol_Idle"));
    SkeletalMeshComponent->SetSkeletalMeshAsset(SkeletalMesh);
    BindSelfLuaProperties();
    SkeletalMeshComponent->StateMachineFileName = "LuaScripts/Animations/EnemyStateMachine.lua";


    SetActorLocation(GetOwner()->GetRootComponent()->GetRelativeLocation());
    GetActorRotation();


    CurrentFireTimer = 0.0f;

    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distrib(1.0f, 10.0f);
        FireInterval = distrib(gen);
    }
}

void AEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AEnemy::Destroyed()
{
    Super::Destroyed();
    AEnemySpawner* Owner = Cast<AEnemySpawner>(GetOwner());


    SkeletalMeshComponent->bSimulate = true;
    //Owner->SpawnedEnemy = nullptr;

    // rag doll
}

void AEnemy::CalculateTimer(float DeltaTime)
{
    CurrentFireTimer += DeltaTime;

    if (CurrentFireTimer >= FireInterval)
    {
        CurrentFireTimer = 0.f;

        bShouldFire = true;
    }
}

void AEnemy::Fire()
{
    UWorld* World = GEngine->ActiveWorld;

    ABullet* Bullet = World->SpawnActor<ABullet>();
    Bullet->SetActorLabel(TEXT("OBJ_BULLET"));
    Bullet->SetOwner(this);

    bShouldFire = false;
}
