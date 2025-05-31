#pragma once
#include "Engine.h"
#include "Actors/Player.h"
#include "World/SkeletalViewerWorld.h"
#include "World/PhysicsViewerWorld.h"

/*
    Editor 모드에서 사용될 엔진.
    UEngine을 상속받아 Editor에서 사용 될 기능 구현.
    내부적으로 PIE, Editor World 두 가지 형태로 관리.
*/

class UParticleViewerWorld;
class UParticleSystem;
class AActor;
class USceneComponent;
class UPhysicsAsset;

enum class EViewerType : uint8
{
    EVT_Editor,
    EVT_PIE,
    EVT_SkeletalMeshViewer,
    EVT_ParticleViewer,
    EVT_PhysicsAssetViewer,
    EVT_MAX,
};

class UEditorEngine : public UEngine
{
    DECLARE_CLASS(UEditorEngine, UEngine)

public:
    UEditorEngine() = default;

    virtual void Init() override;
    virtual void Tick(float DeltaTime) override;
    virtual void Release() override;

    UWorld* PIEWorld = nullptr;
    USkeletalViewerWorld* SkeletalMeshViewerWorld = nullptr;
    UParticleViewerWorld* ParticleViewerWorld = nullptr;
    UPhysicsAssetViewerWorld* PhysicsAssetViewerWorld = nullptr;
    UWorld* EditorWorld = nullptr;
    
    void StartPIE();
    void StartSkeletalMeshViewer(FName SkeletalMeshName, UAnimationAsset* AnimAsset);
    void StartParticleViewer(UParticleSystem* ParticleSystemAsset);
    void StartPhysicsAssetViewer(FName PreviewMeshKey, FName PhysicsAssetName);

    void BindEssentialObjects();

    void EndPIE();
    void EndSkeletalMeshViewer();
    void EndParticleViewer();
    void EndPhysicsAssetViewer();

    void SetPhysXScene(UWorld* World);

    // 주석은 UE에서 사용하던 매개변수.
    FWorldContext& GetEditorWorldContext(/*bool bEnsureIsGWorld = false*/);
    FWorldContext* GetPIEWorldContext(/*int32 WorldPIEInstance = 0*/);

    EViewerType GetViewerType() const { return ViewerType; }

private:
    EViewerType ViewerType = EViewerType::EVT_Editor;
    
public:
    void SelectActor(AActor* InActor);

    // 전달된 액터가 선택된 컴포넌트와 같다면 해제 
    void DeselectActor(AActor* InActor);
    void ClearActorSelection(); 
    
    bool CanSelectActor(const AActor* InActor) const;
    AActor* GetSelectedActor() const;

    void HoverActor(AActor* InActor);
    
    void NewLevel();

    void SelectComponent(USceneComponent* InComponent) const;
    
    // 전달된 InComponent가 현재 선택된 컴포넌트와 같다면 선택 해제
    void DeselectComponent(USceneComponent* InComponent);
    void ClearComponentSelection(); 
    
    bool CanSelectComponent(const USceneComponent* InComponent) const;
    USceneComponent* GetSelectedComponent() const;

    void HoverComponent(USceneComponent* InComponent);

public:
    AEditorPlayer* GetEditorPlayer() const;
    
private:
    AEditorPlayer* EditorPlayer = nullptr;
    FVector CameraLocation;
    FVector CameraRotation;
};



