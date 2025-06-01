#include "BulletTester.h"
#include "World/World.h"    // GetWorld()를 사용하기 위해
#include "Engine/EditorEngine.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"
#include "Launch/EngineLoop.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

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
            FVector CamLoc = FVector::ZeroVector;
            FRotator CamRot = FRotator::ZeroRotator;

            // === 필수 수정 1: 플레이어 카메라 정보 얻기 ===
            APlayerController* PC = World->GetPlayerController();
            if (PC && PC->PlayerCameraManager) // 플레이어 컨트롤러와 카메라 매니저가 유효한지 확인합니다.
            {
                AActor* ViewActor = PC->PlayerCameraManager->GetViewTarget(); // 카메라가 보고 있는 대상 액터를 가져옵니다.
                if (ViewActor) // ViewActor가 유효하다면 그 액터의 위치와 회전을 사용합니다.
                {
                    CamLoc = ViewActor->GetActorLocation();
                    CamRot = ViewActor->GetActorRotation();
                }
            }
            else // 플레이어 컨트롤러가 없다면, ABulletTester 자신의 위치를 기준으로 합니다.
            {
                CamLoc = GetActorLocation();
                CamRot = GetActorRotation();
            }

            FVector ForwardVector = CamRot.RotateVector(FVector(1.0f, 0.0f, 0.0f));
            FVector SpawnLocation = CamLoc + ForwardVector * 100.0f;
            FRotator SpawnRotation = CamRot;

            // (UClass* InClass, FName InActorName) 오버로드를 사용하고,
            // 스폰된 액터의 위치와 회전을 수동으로 설정해야 합니다.
            AActor* SpawnedRawActor = World->SpawnActor(BulletClass);
            ABullet* NewBullet = Cast<ABullet>(SpawnedRawActor); // SpawnActor가 반환한 AActor*를 ABullet*으로 캐스팅합니다.

            if (NewBullet)
            {
                // 스폰된 액터의 위치와 회전을 수동으로 설정합니다.
                NewBullet->SetActorLocation(SpawnLocation);
                NewBullet->SetActorRotation(SpawnRotation);
                
                // === 선택적 수정 1: ProjectileMovement 컴포넌트 속성 재설정 (ABullet의 기본값을 덮어쓸 때) ===
                // ABullet의 생성자에서 이미 기본값을 설정했지만, ABulletTester에서 이를 덮어쓸 수도 있습니다.
                // (이 값들이 현재 움직임에 영향을 주지는 않지만, 데이터는 설정됩니다.)
                if (NewBullet->GetProjectileMovement())
                {
                    // SetInitialSpeed는 Velocity를 설정하지만, TickComponent가 호출되지 않으므로 영향 없음.
                    NewBullet->GetProjectileMovement()->SetInitialSpeed(1000.0f);
                    // SetMaxSpeed, SetGravity, SetLifetime도 마찬가지로 영향 없음.
                    NewBullet->GetProjectileMovement()->SetMaxSpeed(2000.0f);
                    // UProjectileMovementComponent에서 중력이 Velocity.Z += Gravity * DeltaTime; 이므로
                    // Z축이 위라면, 아래로 작용하려면 Gravity 값을 음수로 설정해야 합니다.
                    NewBullet->GetProjectileMovement()->SetGravity(-980.0f); // 중요: Z축이 위일 경우 음수
                    NewBullet->GetProjectileMovement()->SetLifetime(3.0f);   // 3초 후 파괴 (Tick이 없어 파괴되지 않을 것임)
                }

                // 총알을 특정 방향으로 발사합니다. (이 호출은 ProjectileMovement의 Velocity를 설정하지만, 움직이지는 않을 것임)
                FVector InitialVelocity = ForwardVector * 1000.0f;
                NewBullet->FireInDirection(InitialVelocity);

                // (선택) 에디터 상에서 총알 선택 상태로 만들기
                UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
                if (Engine)
                {
                    Engine->DeselectComponent(Engine->GetSelectedComponent());
                    Engine->SelectActor(NewBullet);
                }
            }
        }
        bHasFired = true;
    }
}

