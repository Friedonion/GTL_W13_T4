#include "Bullet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ProjectileMovementComponent.h"

#include "World/World.h"
#include "Engine/EditorEngine.h"
#include "LevelEditor/SLevelEditor.h"
#include "UnrealEd/EditorViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

#include "Math/MathUtility.h"

ABullet::ABullet()
{
    // Tick 을 사용할 수 있게 허용 (이 액터가 Tick() 을 호출받도록)
    // PrimaryActorTick.bCanEverTick = true;

    // 메시 컴포넌트 생성 -> BulletMesh 에 저장 -> 루트 컴포넌트로 설정
    BulletMesh = AddComponent<UStaticMeshComponent>(TEXT("BulletMesh"));
    SetRootComponent(BulletMesh);

    // === 3. BulletMesh에 스태틱 메시 할당 (총알이 보이게 하려면 필수!) ===
    if (BulletMesh)
    {
        // "Contents/Sphere.obj"는 예시 경로이며, 실제 엔진의 에셋 로딩 시스템에 맞는 경로를 사용해야 합니다.
        // BulletMesh->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Sphere.obj"));
        BulletMesh->SetStaticMesh(nullptr); // 임시로 nullptr로 설정하면 컴파일은 되지만 아무것도 보이지 않습니다.

    }

    // 이동 컴포넌트를 생성 -> ProjectileMovement 에 저장
    ProjectileMovement = AddComponent<UProjectileMovementComponent>(TEXT("BulletMovement"));

    if (ProjectileMovement)
    {
        ProjectileMovement->SetLifetime(3.0f);   // 총알 생명주기 기본 3초 (3초 후 파괴 로직이 작동하겠지만, Tick이 없어 파괴되지 않을 것임)
        ProjectileMovement->SetMaxSpeed(2000.0f); // 최대 속도 2000
        // 중력 값 설정: UProjectileMovementComponent에서 Velocity.Z += Gravity * DeltaTime; 이므로,
        // 만약 Z축이 '위'라면 중력이 '아래'로 작용하게 하려면 Gravity 값을 음수로 설정해야 합니다.
        // (마찬가지로 Tick이 없어 실제 움직임에 영향은 없지만, 올바른 값 설정)
        ProjectileMovement->SetGravity(-980.0f); // Z축이 '위'일 경우 음수 중력 가속도.
    }
}

ABullet::~ABullet()
{
}

void ABullet::BeginPlay()
{
    Super::BeginPlay();
}

void ABullet::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // 이 함수는 'PrimaryActorTick.bCanEverTick'이 정의되지 않아 호출되지 않을 것입니다.
    // 따라서 UProjectileMovementComponent::TickComponent 또한 자동으로 호출되지 않습니다.
}

void ABullet::FireInDirection(const FVector& InitialVelocity)
{
    if (ProjectileMovement)
    {
        // ProjectileMovement 컴포넌트가 제공하는 SetVelocity 메서드를 호출
        // ProjectileMovement 컴포넌트의 InitialVelocity를 설정합니다.
        // (마찬가지로 Tick이 없어 실제 움직임에 영향은 없지만, 값은 설정됩니다.)
        ProjectileMovement->SetVelocity(InitialVelocity);
    }
}

ABullet* ABullet::SpawnBulletFromCurrentViewpoint(UWorld* InWorld)
{
    if (!InWorld) // 월드가 유효한지 먼저 확인합니다.
    {
        return nullptr;
    }

    FVector CamLoc = FVector::ZeroVector;
    FRotator CamRot = FRotator::ZeroRotator;

    // 플레이어 컨트롤러/카메라 매니저 또는 에디터 카메라에서 뷰포인트 얻기
    APlayerController* PC = InWorld->GetPlayerController();
    if (PC && PC->PlayerCameraManager)
    {
        AActor* ViewActor = PC->PlayerCameraManager->GetViewTarget();
        if (ViewActor)
        {
            CamLoc = ViewActor->GetActorLocation();
            CamRot = ViewActor->GetActorRotation();
        }
    }

    // Fallback: 플레이어 컨트롤러나 뷰 타겟이 없으면 에디터 뷰포트 카메라를 사용합니다.
    // CamLoc과 CamRot이 기본값(ZeroVector/ZeroRotator)이라면 아직 설정되지 않았다는 의미로 사용합니다.
    if (CamLoc == FVector::ZeroVector && CamRot == FRotator::ZeroRotator)
    {
        // GEngineLoop.GetLevelEditor()와 GetActiveViewportClient()가 유효하다고 가정합니다.
        if (GEngineLoop.GetLevelEditor() && GEngineLoop.GetLevelEditor()->GetActiveViewportClient())
        {
            std::shared_ptr<FEditorViewportClient> Client = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();

            CamLoc = Client->GetCameraLocation();

            const FVector* CameraRotationVector = nullptr;

            if (Client->IsOrthographic())
            {
                CameraRotationVector = &Client->OrthogonalCamera.GetRotation();
            }
            else
            {
                CameraRotationVector = &Client->PerspectiveCamera.GetRotation();
            }

            if (CameraRotationVector)
            {
                // FVector의 X, Y, Z가 Pitch, Yaw, Roll에 매핑된다고 가정하고 FRotator로 변환합니다.
                CamRot = FRotator(CameraRotationVector->X, CameraRotationVector->Y, CameraRotationVector->Z);
            }
            else
            {
                CamRot = FRotator::ZeroRotator;
            }
        }
        else
        {
            // 에디터 뷰포트도 가져올 수 없는 최후의 경우, (0,0,0)에 스폰됩니다.
            // 이 경우 CamLoc과 CamRot은 ZeroVector/ZeroRotator로 유지됩니다.
        }
    }

    // 카메라 전방 벡터를 계산합니다.
    FVector CamForward = CamRot.RotateVector(FVector(1.0f, 0.0f, 0.0f));
    // 총알 스폰 위치를 카메라 위치에서 전방으로 100.0f 떨어진 곳으로 설정합니다.
    FVector SpawnLoc = CamLoc + CamForward * 100.0f;
    // 총알 스폰 회전은 카메라 회전과 동일하게 설정합니다.
    FRotator SpawnRot = CamRot;

    // 월드에 ABullet 액터를 스폰합니다.
    ABullet* BulletActor = InWorld->SpawnActor<ABullet>();

    if (BulletActor)
    {
        // 스폰된 총알 액터의 위치와 회전을 설정합니다.
        BulletActor->SetActorLocation(SpawnLoc);
        BulletActor->SetActorRotation(SpawnRot);
        // 총알을 지정된 방향(CamForward)과 속도(1000.0f)로 발사합니다.
        BulletActor->FireInDirection(CamForward * 1000.0f);
    }

    return BulletActor; // 스폰된 총알 액터를 반환합니다.
}
