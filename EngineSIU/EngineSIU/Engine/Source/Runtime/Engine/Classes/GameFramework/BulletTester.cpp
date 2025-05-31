#include "BulletTester.h"
#include "World/World.h"    // GetWorld()를 사용하기 위해
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

ABulletTester::ABulletTester()
{
    bHasFired = false; // 처음에 아직 발사되지 않음

    BulletClass = ABullet::StaticClass();
}

ABulletTester::~ABulletTester()
{
}

void ABulletTester::BeginPlay()
{
    Super::BeginPlay();

    if (!bHasFired && BulletClass)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            ABullet* NewBullet = World->SpawnActor<ABullet>();
            if (NewBullet)
            {
                FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 100.0f;
                FRotator SpawnRotation = GetActorRotation();
                NewBullet->SetActorLocation(SpawnLocation);
                NewBullet->SetActorRotation(SpawnRotation);

                FVector InitialVelocity = GetActorForwardVector() * 1000.0f;
                NewBullet->FireInDirection(InitialVelocity);
            }
        }

        bHasFired = true;
    }
}
