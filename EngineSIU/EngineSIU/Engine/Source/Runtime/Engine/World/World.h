#pragma once
#include "Define.h"
#include "Container/Set.h"
#include "UObject/ObjectFactory.h"
#include "UObject/ObjectMacros.h"
#include "WorldType.h"
#include "Level.h"
#include "GameFramework/PlayerController.h"
#include "Engine/EventManager.h"
#include "UObject/UObjectIterator.h"

class UPrimitiveComponent;
struct FOverlapResult;
class UCameraComponent;
class FObjectFactory;
class AActor;
class UObject;
class USceneComponent;
class FCollisionManager;
class AGameMode;
class UTextComponent;
class APlayerCharacter;

class UWorld : public UObject
{
    DECLARE_CLASS(UWorld, UObject)

public:
    UWorld() = default;
    virtual ~UWorld() override;

    static UWorld* CreateWorld(UObject* InOuter, const EWorldType InWorldType, const FString& InWorldName = "DefaultWorld");

    void InitializeNewWorld();
    virtual UObject* Duplicate(UObject* InOuter) override;

    virtual void Tick(float DeltaTime);
    void BeginPlay();

    void Release();

    /**
     * World에 Actor를 Spawn합니다.
     * @param InClass Spawn할 Actor 정보
     * @return Spawn된 Actor
     */
    AActor* SpawnActor(UClass* InClass, FName InActorName = NAME_None);

    /** 
     * World에 Actor를 Spawn합니다.
     * @tparam T AActor를 상속받은 클래스
     * @return Spawn된 Actor의 포인터
     */
    template <typename T>
        requires std::derived_from<T, AActor>
    T* SpawnActor();

    /** World에 존재하는 Actor를 제거합니다. */
    bool DestroyActor(AActor* ThisActor);

    virtual UWorld* GetWorld() const override;
    ULevel* GetActiveLevel() const { return ActiveLevel; }

    template <typename T>
        requires std::derived_from<T, AActor>
    T* DuplicateActor(T* InActor);

    EWorldType WorldType = EWorldType::None;
    
    FEventManager EventManager;

    void SetMainPlayer(APlayerCharacter* InPlayer){ MainPlayer = InPlayer; }
    APlayerCharacter* GetMainPlayer() const;

    void SetPlayerController(APlayerController* InPlayerController){ PlayerController = InPlayerController; }
    APlayerController* GetPlayerController() const;

    AGameMode* GetGameMode() const { return GameMode; }
    
    void CheckOverlap(const UPrimitiveComponent* Component, TArray<FOverlapResult>& OutOverlaps) const;

public:
    double TimeSeconds = 0.f;

protected:
    
    FString WorldName = "DefaultWorld";
    
private:
    AGameMode* GameMode = nullptr;


    ULevel* ActiveLevel = nullptr;

    /** Actor가 Spawn되었고, 아직 BeginPlay가 호출되지 않은 Actor들 */
    TArray<AActor*> PendingBeginPlayActors;

    // TODO: 싱글 플레이어면 상관 없지만, 로컬 멀티 플레이어인 경우를 위해 배열로 관리하는 방법을 고려하기.
    APlayerController* PlayerController = nullptr;

    APlayerCharacter* MainPlayer = nullptr;

    UTextComponent* MainTextComponent = nullptr;

    FCollisionManager* CollisionManager = nullptr;
};


template <typename T>
    requires std::derived_from<T, AActor>
T* UWorld::SpawnActor()
{
    return Cast<T>(SpawnActor(T::StaticClass()));
}

template <typename T>
    requires std::derived_from<T, AActor>
T* UWorld::DuplicateActor(T* InActor)
{
    if (ULevel* ActiveLevel = GetActiveLevel())
    {
        T* NewActor = static_cast<T*>(InActor->Duplicate(this));
        ActiveLevel->Actors.Add(NewActor);
        PendingBeginPlayActors.Add(NewActor);
        return NewActor;
    }
    return nullptr;
}

