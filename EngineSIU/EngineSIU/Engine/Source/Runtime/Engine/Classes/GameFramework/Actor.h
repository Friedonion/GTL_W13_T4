#pragma once
#include "Components/SceneComponent.h"
#include "Container/Set.h"
#include "Engine/EngineTypes.h"
#include "UObject/Casts.h"
#include "UObject/Object.h"
#include "UObject/ObjectFactory.h"
#include "UObject/ObjectMacros.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FActorBeginOverlapSignature, AActor* /* OverlappedActor */, AActor* /* OtherActor */);
DECLARE_MULTICAST_DELEGATE_TwoParams(FActorEndOverlapSignature, AActor* /* OverlappedActor */, AActor* /* OtherActor */);
DECLARE_MULTICAST_DELEGATE_FourParams(FActorHitSignature, AActor* /* SelfActor */, AActor* /* OtherActor */, FVector /*NormalImpulse*/, const FHitResult& /* Hit */);

class UActorComponent;
class ULuaScriptComponent;

namespace sol
{
class state;
}

class AActor : public UObject
{
    DECLARE_CLASS(AActor, UObject)

public:
    AActor();

    virtual void PostSpawnInitialize();

    virtual UObject* Duplicate(UObject* InOuter) override;

    /** Actor가 게임에 배치되거나 스폰될 때 호출됩니다. */
    virtual void BeginPlay();

    /** 매 Tick마다 호출됩니다. */
    virtual void Tick(float DeltaTime);

    /** Actor가 제거될 때 호출됩니다. */
    virtual void Destroyed();

    /**
     * 액터가 게임 플레이를 종료할 때 호출되는 함수입니다.
     *
     * @param EndPlayReason EndPlay가 호출된 이유를 나타내는 열거형 값
     * @note Destroyed와는 다른점은, EndPlay는 레벨 전환, 게임 종료, 또는 Destroy() 호출 시 항상 실행됩니다.
     */
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

    bool IsOverlappingActor(const AActor* Other) const;

public:
    /** 이 Actor를 제거합니다. */
    virtual bool Destroy();

    /** 현재 Actor가 제거중인지 여부를 반환합니다. */
    bool IsActorBeingDestroyed() const
    {
        return bActorIsBeingDestroyed;
    }

    /**
     * Actor에 컴포넌트를 새로 추가합니다.
     * @tparam T UActorComponent를 상속받은 Component
     * @return 생성된 Component
     */
    template <typename T>
        requires std::derived_from<T, UActorComponent>
    T* AddComponent(FName InName = NAME_None);
    UActorComponent* AddComponent(UClass* InClass, FName InName = NAME_None, bool bTryRootComponent = true);


    /** Actor가 가지고 있는 Component를 제거합니다. */
    void RemoveOwnedComponent(UActorComponent* Component);

    /** Actor가 가지고 있는 모든 컴포넌트를 가져옵니다. */
    const TSet<UActorComponent*>& GetComponents() const { return OwnedComponents; }

    template<typename T>
        requires std::derived_from<T, UActorComponent>
    T* GetComponentByClass() const;
    template <typename T> requires std::derived_from<T, UActorComponent>
    TArray<T*> GetComponentsByClass() const;
    

    void InitializeComponents();
    void UninitializeComponents();

public:
    USceneComponent* GetRootComponent() const { return RootComponent; }
    bool SetRootComponent(USceneComponent* NewRootComponent);

    AActor* GetOwner() const { return Owner; }
    void SetOwner(AActor* NewOwner) { Owner = NewOwner; }

public:
    FVector GetActorLocation() const;
    FRotator GetActorRotation() const;
    FVector GetActorScale() const;

    FVector GetActorForwardVector() const { return RootComponent ? RootComponent->GetForwardVector() : FVector::ForwardVector; }
    FVector GetActorRightVector() const { return RootComponent ? RootComponent->GetRightVector() : FVector::RightVector; }
    FVector GetActorUpVector() const { return RootComponent ? RootComponent->GetUpVector() : FVector::UpVector; }

    bool SetActorLocation(const FVector& NewLocation);
    bool SetActorRotation(const FRotator& NewRotation);
    bool SetActorScale(const FVector& NewScale);

    bool AddActorLocation(const FVector& DeltaLocation);
    bool AddActorRotation(const FRotator& DeltaRotation);
    bool AddActorRotation(const FQuat& DeltaRotation);
    bool AddActorScale(const FVector& DeltaScale);

protected:
    UPROPERTY
    (USceneComponent*, RootComponent, = nullptr)

private:
    /** 이 Actor를 소유하고 있는 다른 Actor의 정보 */
    UPROPERTY
    (AActor*, Owner, = nullptr)

    /** 본인이 소유하고 있는 컴포넌트들의 정보 */
    TSet<UActorComponent*> OwnedComponents;


    /** 현재 Actor가 삭제 처리중인지 여부 */
    uint8 bActorIsBeingDestroyed : 1 = false;

#if 1 // TODO: WITH_EDITOR 추가
public:
    /** Actor의 기본 Label을 가져옵니다. */
    FString GetDefaultActorLabel() const;

    /** Actor의 Label을 가져옵니다. */
    FString GetActorLabel() const;

    /** Actor의 Label을 설정합니다. */
    void SetActorLabel(const FString& NewActorLabel, bool bUUID = true);

private:
    /** 에디터상에 보이는 Actor의 이름 */
    UPROPERTY
    (FString, ActorLabel)
#endif

public:
    bool IsActorTickInEditor() const { return bTickInEditor; }
    void SetActorTickInEditor(bool InbInTickInEditor);

    bool IsHidden() const { return bHidden; }
    void SetHidden(bool InbHidden) { bHidden = InbHidden; }

private:
    bool bTickInEditor = false;     // Editor Tick을 수행 여부

    bool bHidden = false;
    
public:
    /** 
     * Called when another actor begins to overlap this actor, for example a player walking into a trigger.
     * For events when objects have a blocking collision, for example a player hitting a wall, see 'Hit' events.
     * @note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
     */
    FActorBeginOverlapSignature OnActorBeginOverlap;

    /** 
     * Called when another actor stops overlapping this actor. 
     * @note Components on both this and the other Actor must have bGenerateOverlapEvents set to true to generate overlap events.
     */
    FActorEndOverlapSignature OnActorEndOverlap;

    /** 
     *    Called when this Actor hits (or is hit by) something solid. This could happen due to things like Character movement, using Set Location with 'sweep' enabled, or physics simulation.
     *    For events when objects overlap (e.g. walking into a trigger) see the 'Overlap' event.
     *    @note For collisions during physics simulation to generate hit events, 'Simulation Generates Hit Events' must be enabled.
     */
    FActorHitSignature OnActorHit;


public:
    void InitLuaScriptComponent();
    virtual void RegisterLuaType(sol::state& Lua); // Lua에 클래스 등록해주는 함수.
    virtual bool BindSelfLuaProperties(); // LuaEnv에서 사용할 멤버 변수 등록 함수.
    ULuaScriptComponent* GetLuaScriptComponent() const { return LuaScriptComponent; } 
    
private:
    UPROPERTY(EditAnywhere | EditInline, ULuaScriptComponent*, LuaScriptComponent, = nullptr)
    UPROPERTY(EditAnywhere, bool, bUseScript, = true)
    
};

template <typename T>
    requires std::derived_from<T, UActorComponent>
T* AActor::AddComponent(FName InName)
{
    return Cast<T>(AddComponent(T::StaticClass(), InName));
}

template <typename T> requires std::derived_from<T, UActorComponent>
T* AActor::GetComponentByClass() const
{
    for (UActorComponent* Component : OwnedComponents)
    {
        if (T* CastedComponent = Cast<T>(Component))
        {
            return CastedComponent;
        }
    }
    return nullptr;
}

template <typename T> requires std::derived_from<T, UActorComponent>
TArray<T*> AActor::GetComponentsByClass() const
{
    TArray<T*> Components;
    
    for (UActorComponent* Component : OwnedComponents)
    {
        if (T* CastedComponent = Cast<T>(Component))
        {
            Components.Add(CastedComponent);
        }
    }
    return Components;
}
