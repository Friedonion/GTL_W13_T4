#pragma once
#include "World.h"

class USkeletalMeshComponent;

class UPhysicsAssetViewerWorld : public UWorld
{
    DECLARE_CLASS(UPhysicsAssetViewerWorld, UWorld)
public:
    UPhysicsAssetViewerWorld() = default;

    static UPhysicsAssetViewerWorld* CreateWorld(UObject* InOuter,
        const EWorldType InWorldType = EWorldType::PhysicsAssetViewer,
        const FString& InWorldName = "PhysicsAssetViewerWorld"
    );

    void SetSkeletalMeshComponent(USkeletalMeshComponent* Component)
    {
        // 이 함수가 호출되면, SkeletalMeshComponent 변수에 넘어온 Component 값을 저장합니다.
        // 'this->' 는 '이 클래스(UPhysicsAssetViewerWorld)의' 라는 의미입니다.
        this->CurrentSkeletalMeshComponent = Component;
    }

    USkeletalMeshComponent* GetSkeletalMeshComponent()
    {
        // 저장된 SkeletalMeshComponent를 돌려줍니다.
        return CurrentSkeletalMeshComponent;
    }

    void Tick(float DeltaTime) override;

private:
    USkeletalMeshComponent* CurrentSkeletalMeshComponent = nullptr; //
};

