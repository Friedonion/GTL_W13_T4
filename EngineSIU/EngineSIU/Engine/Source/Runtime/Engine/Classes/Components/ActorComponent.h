#pragma once
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class AActor;

class UActorComponent : public UObject
{
    DECLARE_CLASS(UActorComponent, UObject)

private:
    friend class AActor;

public:
    UActorComponent() = default;

    virtual UObject* Duplicate(UObject* InOuter) override;

    /**
* 이 컴포넌트의 직렬화 가능한 속성들을 문자열 맵으로 반환합니다.
* 하위 클래스는 이 함수를 재정의하여 자신만의 속성을 추가해야 합니다.
*/
    virtual void GetProperties(TMap<FString, FString>& OutProperties) const;

    /** 저장된 Properties 맵에서 컴포넌트의 상태를 복원합니다. */
    virtual void SetProperties(const TMap<FString, FString>& Properties);


    /** AActor가 World에 Spawn되어 BeginPlay이전에 호출됩니다. */
    virtual void InitializeComponent();

    /** 컴포넌트가 제거되거나 소유자 Actor가 파괴될 때 호출됩니다. */
    virtual void UninitializeComponent();

    /** 모든 초기화가 끝나고, 준비가 되었을 때 호출됩니다. */
    virtual void BeginPlay();

    /** 매 틱마다 호출됩니다. */
    virtual void TickComponent(float DeltaTime);

    virtual void EndPhysicsTickComponent(float DeltaTime);

    /** Component가 제거되었을 때 호출됩니다. */
    virtual void OnComponentDestroyed();

    /**
     * Ends gameplay for this component.
     * Called from AActor::EndPlay only
     */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

public:
    /** 이 컴포넌트를 소유하고 있는 Actor를 반환합니다. */
    AActor* GetOwner() const { return OwnerPrivate; }
    void SetOwner(AActor* InOwner) { OwnerPrivate = InOwner; }

    /** 이 컴포넌트를 제거합니다. */
    virtual void DestroyComponent(bool bPromoteChildren = false);

    /** Component의 BeginPlay가 호출 되었는지 여부를 반환합니다. */
    bool HasBegunPlay() const { return bHasBegunPlay; }

    /** Component가 초기화 되었는지 여부를 반환합니다. */
    bool HasBeenInitialized() const { return bHasBeenInitialized; }

    /** Component가 현재 활성화 중인지 여부를 반환합니다. */
    bool IsActive() const { return bIsActive; }

    void Activate();
    void Deactivate();

private:
    AActor* OwnerPrivate;

    /** InitializeComponent가 호출 되었는지 여부 */
    uint8 bHasBeenInitialized : 1 = false;

    /** BeginPlay가 호출 되었는지 여부 */
    uint8 bHasBegunPlay : 1 = false;

    /** 현재 컴포넌트가 삭제 처리중인지 여부 */
    uint8 bIsBeingDestroyed : 1 = false;

    /** Component가 현재 활성화 중인지 여부 */
    uint8 bIsActive : 1 = true;

public:
    /** Component가 초기화 되었을 때, 자동으로 활성화할지 여부 */
    uint8 bAutoActive : 1 = true;
};
