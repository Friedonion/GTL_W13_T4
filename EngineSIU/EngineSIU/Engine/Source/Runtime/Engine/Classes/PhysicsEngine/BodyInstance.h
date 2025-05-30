#pragma once
#include <PxRigidActor.h>
#include "UObject/ObjectMacros.h"

class UPrimitiveComponent;
struct GameObject;

enum class ECollisionEnabled
{
    NoCollision, // 충돌 없음
    QueryOnly, // 쿼리만 (트레이스, 오버랩)
    PhysicsOnly, // 물리만 (시뮬레이션)
    QueryAndPhysics, // 둘 다
};

struct FBodyInstance
{
    DECLARE_STRUCT(FBodyInstance)
    FBodyInstance(UPrimitiveComponent* InOwner); // TODO: 초기값 설정?

    void SetGameObject(GameObject* InGameObject);

    // BodyInstance Name
    UPROPERTY_WITH_FLAGS(EditAnywhere, FName, BodyInstanceName)

    // ==================== 질량과 관성 ====================
    
    /** 바디의 질량 (킬로그램) */
    UPROPERTY_WITH_FLAGS(EditAnywhere, float, MassInKg, = 100.0f)
    
    /** 질량 중심 오프셋 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, FVector, COMNudge)
    
    /** 관성 텐서 스케일 (회전 저항) */
    UPROPERTY_WITH_FLAGS(EditAnywhere, FVector, InertiaTensorScale, = FVector(3.0f, 3.0f, 3.0f))
    
    // ==================== 시뮬레이션 설정 ====================
    
    /** 물리 시뮬레이션을 할지 여부 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bSimulatePhysics, = true)
    
    /** 중력을 적용할지 여부 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bEnableGravity, = true)
    
    /** 자동으로 인접한 바디와 용접할지 여부 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bAutoWeld, = true)
    
    /** 시작할 때 깨어있는 상태인지 (잠들어있으면 CPU 절약) */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bStartAwake, = true)
    
    // ==================== 움직임 제한 ====================
    
    /** X축 이동 잠금 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bLockXTranslation, = false)
    
    /** Y축 이동 잠금 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bLockYTranslation, = false)
    
    /** Z축 이동 잠금 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bLockZTranslation, = false)
    
    /** X축 회전 잠금 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bLockXRotation, = false)
    
    /** Y축 회전 잠금 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bLockYRotation, = false)
    
    /** Z축 회전 잠금 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bLockZRotation, = false)
    
    // ==================== 댐핑 (저항) ====================
    
    /** 선형 댐핑 - 이동 속도 감소율 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, float, LinearDamping, = 0.1f)
    
    /** 각속도 댐핑 - 회전 속도 감소율 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, float, AngularDamping, = 0.1f)
    
    // ==================== 충돌 설정 (TODO: 충돌 시스템 구현 후 추가) ====================
    
    /** 충돌 활성화 타입 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, ECollisionEnabled, CollisionEnabled)
    // - NoCollision: 충돌 없음
    // - QueryOnly: 쿼리만 (트레이스, 오버랩)
    // - PhysicsOnly: 물리만 (시뮬레이션)
    // - QueryAndPhysics: 둘 다
    
    /** 충돌 오브젝트 타입 */
    // UPROPERTY_WITH_FLAGS(EditAnywhere, ECollisionChannel, ObjectType)
    
    /** 각 충돌 채널에 대한 응답 */
    // UPROPERTY_WITH_FLAGS(EditAnywhere, FCollisionResponseContainer, CollisionResponses)
    
    // ==================== 고급 물리 설정 ====================
    
    /** 연속 충돌 검출 (CCD) 활성화 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bUseCCD, = true)
    
    /** 복잡한 충돌을 단순 충돌로 사용할지 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bUseComplexAsSimpleCollision, = true)
    
    /** 비동기 물리 시뮬레이션 사용 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, bool, bUseAsyncScene, = true)
    
    /** 최대 각속도 제한 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, float, MaxAngularVelocity, = 100)
    
    /** 위치 솔버 반복 횟수 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, uint8, PositionSolverIterationCount, = 8)
    
    /** 속도 솔버 반복 횟수 */
    UPROPERTY_WITH_FLAGS(EditAnywhere, uint8, VelocitySolverIterationCount, = 8)
    
    
    // PhysX 객체 참조
    physx::PxRigidActor* RigidActorSync = nullptr;   // 동기 액터
    physx::PxRigidActor* RigidActorAsync = nullptr;  // 비동기 액터

    GameObject* BIGameObject = nullptr;

    UPrimitiveComponent* OwnerComponent = nullptr;
    
    int32 BoneIndex = -1;
};
