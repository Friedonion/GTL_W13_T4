#pragma once
#include "GameFramework/Actor.h"

class UStaticMesh;
class UStaticMeshComponent;
class GameObject;

class AFist : public AActor
{
    DECLARE_CLASS(AFist, AActor)

public:
    AFist();
    virtual void PostSpawnInitialize() override;
    virtual UObject* Duplicate(UObject* InOuter) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Destroyed() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    bool bVisible = true;
    float InitialSpeed = 0.3f;
    float WaitTime = 0.f;
    bool bSpawned = true;
    float Lifetime = 0.5f; // Fist의 생명주기

    void SetShooter(AActor* InShooter)
    {
        Shooter = InShooter;
    }
private:
    UPROPERTY(EditAnywhere, UStaticMeshComponent*, StaticMeshComponent, = nullptr)

    GameObject* GameObject = nullptr; // Fist의 게임 오브젝트

    void SpawnObject();
    
    AActor* Shooter = nullptr;
};
