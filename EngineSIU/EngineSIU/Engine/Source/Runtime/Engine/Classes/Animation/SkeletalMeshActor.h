#pragma once
#include "GameFramework/Actor.h"

class USkeletalMeshComponent;

class ASkeletalMeshActor : public AActor
{
    DECLARE_CLASS(ASkeletalMeshActor, AActor)

public:
    ASkeletalMeshActor() = default;
    virtual ~ASkeletalMeshActor() override = default;

    virtual void PostSpawnInitialize() override;

    virtual void BeginPlay() override;
    virtual UObject* Duplicate(UObject* InOuter) override;

    UPROPERTY(EditAnywhere | EditInline, USkeletalMeshComponent*, SkeletalMeshComponent, = nullptr)
    
};
