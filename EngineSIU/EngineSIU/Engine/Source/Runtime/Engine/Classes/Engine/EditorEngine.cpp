#include "EditorEngine.h"

#include "FbxLoader.h"
#include "FObjLoader.h"
#include "World/World.h"
#include "Level.h"
#include "Animation/SkeletalMeshActor.h"
#include "GameFramework/Actor.h"
#include "Classes/Engine/AssetManager.h"
#include "UObject/UObjectIterator.h"
#include "Actors/DirectionalLightActor.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include "Particles/ParticleSystemComponent.h"
#include "PropertyEditor/ParticleViewerPanel.h"
#include "UnrealEd/UnrealEd.h"
#include "World/ParticleViewerWorld.h"
#include "Physics/PhysicsManager.h"
#include "SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Particles/ParticleSystem.h"

extern FEngineLoop GEngineLoop;

namespace PrivateEditorSelection
{
    static AActor* GActorSelected = nullptr;
    static AActor* GActorHovered = nullptr;

    static USceneComponent* GComponentSelected = nullptr;
    static USceneComponent* GComponentHovered = nullptr;
}

void UEditorEngine::Init()
{
    Super::Init();

    // Initialize the engine
    GEngine = this;

    FWorldContext& EditorWorldContext = CreateNewWorldContext(EWorldType::Editor);

    EditorWorld = UWorld::CreateWorld(this, EWorldType::Editor, FString("EditorWorld"));

    EditorWorldContext.SetCurrentWorld(EditorWorld);
    ActiveWorld = EditorWorld;

    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
    
    LoadLevel("Saved/AutoSaves.scene");
}

void UEditorEngine::Release()
{
    if (ActiveWorld->WorldType == EWorldType::PIE)
    {
        EndPIE();
    }
    if (ActiveWorld->WorldType == EWorldType::SkeletalViewer)
    {
        EndSkeletalMeshViewer();
    }
    if (ActiveWorld->WorldType == EWorldType::ParticleViewer)
    {
        EndParticleViewer();
    }
    if (ActiveWorld->WorldType == EWorldType::PhysicsAssetViewer)
    {
        EndPhysicsAssetViewer();
    }
    
    SaveLevel("Saved/AutoSaves.scene");
    
    for (FWorldContext* WorldContext : WorldList)
    {
        WorldContext->World()->Release();
    }
    WorldList.Empty();
    PhysicsManager->ShutdownPhysX();
    
    Super::Release();
}

void UEditorEngine::Tick(float DeltaTime)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->WorldType == EWorldType::Editor)
        {
            if (UWorld* World = WorldContext->World())
            {
                // TODO: World에서 EditorPlayer 제거 후 Tick 호출 제거 필요.
                World->Tick(DeltaTime);
                EditorPlayer->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {
                    for (AActor* Actor : CachedActors)
                    {
                        if (!Actor)
                        {
                            continue;
                        }

                        const bool bActorTickInEditor = Actor->IsActorTickInEditor();
                        if (bActorTickInEditor)
                        {
                            Actor->Tick(DeltaTime);
                        }

                        for (const auto& Comp : Actor->GetComponents())
                        {
                            // 파티클 컴포넌트는 항상 Tick 호출
                            if (bActorTickInEditor || Comp->IsA<UParticleSystemComponent>())
                            {
                                Comp->TickComponent(DeltaTime);
                            }
                        }

                        if (bActorTickInEditor)
                        {
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->EndPhysicsTickComponent(DeltaTime);
                            }
                        }
                    }
                }
            }
        }
        else if (WorldContext->WorldType == EWorldType::PIE)
        {
            if (UWorld* World = WorldContext->World())
            {
                World->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {

                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            Actor->Tick(DeltaTime);

                            // 물리기반 시뮬레이션을 위한 TickGroup 처리
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->TickComponent(DeltaTime);
                            }
                        }
                    }

                    PhysicsManager->Simulate(DeltaTime);

                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->EndPhysicsTickComponent(DeltaTime);
                            }
                        }
                    }
                    
                }
            }
        }
        else if (WorldContext->WorldType == EWorldType::SkeletalViewer)
        {
            if (UWorld* World = WorldContext->World())
            {
                World->Tick(DeltaTime);
                EditorPlayer->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {
                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            Actor->Tick(DeltaTime);

                            // 물리기반 시뮬레이션을 위한 TickGroup 처리
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->TickComponent(DeltaTime);
                            }
                        }
                    }

                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->EndPhysicsTickComponent(DeltaTime);
                            }
                        }
                    }
                }
            }
        }
        else if (WorldContext->WorldType == EWorldType::ParticleViewer)
        {
            if (UWorld* World = WorldContext->World())
            {
                World->Tick(DeltaTime);
                EditorPlayer->Tick(DeltaTime);
                ULevel* Level = World->GetActiveLevel();
                TArray CachedActors = Level->Actors;
                if (Level)
                {
                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            Actor->Tick(DeltaTime);

                            // 물리기반 시뮬레이션을 위한 TickGroup 처리
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->TickComponent(DeltaTime);
                            }
                        }
                    }

                    for (AActor* Actor : CachedActors)
                    {
                        if (Actor)
                        {
                            for (auto* Comp : Actor->GetComponents())
                            {
                                Comp->EndPhysicsTickComponent(DeltaTime);
                            }
                        }
                    }
                }
            }
        }
    }
}

void UEditorEngine::StartPIE()
{
    if (PIEWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("PIEWorld already exists!"));
        return;
    }

    ViewerType = EViewerType::EVT_PIE;
    
    ClearActorSelection(); // Editor World 기준 Select Actor 해제
    ClearComponentSelection();
    
    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    Handler->OnPIEModeStart();

    FWorldContext& PIEWorldContext = CreateNewWorldContext(EWorldType::PIE);

    PIEWorld = Cast<UWorld>(EditorWorld->Duplicate(this));
    PIEWorld->WorldType = EWorldType::PIE;

    PIEWorldContext.SetCurrentWorld(PIEWorld);
    ActiveWorld = PIEWorld;

    SetPhysXScene(PIEWorld);
    
    BindEssentialObjects();
    
    PIEWorld->BeginPlay();
    // 여기서 Actor들의 BeginPlay를 해줄지 안에서 해줄 지 고민.
    // WorldList.Add(GetWorldContextFromWorld(PIEWorld));
}

void UEditorEngine::StartSkeletalMeshViewer(FName SkeletalMeshName, UAnimationAsset* AnimAsset)
{
    if (SkeletalMeshName == "")
    {
        return;
    }
    if (SkeletalMeshViewerWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("SkeletalMeshViewerWorld already exists!"));
        return;
    }

    ViewerType = EViewerType::EVT_SkeletalMeshViewer;
    
    FWorldContext& WorldContext = CreateNewWorldContext(EWorldType::SkeletalViewer);
    
    SkeletalMeshViewerWorld = USkeletalViewerWorld::CreateWorld(this, EWorldType::SkeletalViewer, FString("SkeletalMeshViewerWorld"));

    WorldContext.SetCurrentWorld(SkeletalMeshViewerWorld);
    ActiveWorld = SkeletalMeshViewerWorld;
    SkeletalMeshViewerWorld->WorldType = EWorldType::SkeletalViewer;

    // 스켈레탈 액터 스폰
    ASkeletalMeshActor* SkeletalActor = SkeletalMeshViewerWorld->SpawnActor<ASkeletalMeshActor>();
    SkeletalActor->SetActorTickInEditor(true);
    
    USkeletalMeshComponent* MeshComp = SkeletalActor->AddComponent<USkeletalMeshComponent>();
    SkeletalActor->SetRootComponent(MeshComp);
    SkeletalActor->SetActorLabel(TEXT("OBJ_SKELETALMESH"));
    MeshComp->SetSkeletalMeshAsset(UAssetManager::Get().GetSkeletalMesh(SkeletalMeshName.ToString()));
    SkeletalMeshViewerWorld->SetSkeletalMeshComponent(MeshComp);

    MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    MeshComp->PlayAnimation(AnimAsset, true);
    MeshComp->DEBUG_SetAnimationEnabled(true);
    MeshComp->SetPlaying(true);
    
    ADirectionalLight* DirectionalLight = SkeletalMeshViewerWorld->SpawnActor<ADirectionalLight>();
    DirectionalLight->SetActorRotation(FRotator(45.f, 45.f, 0.f));
    DirectionalLight->GetComponentByClass<UDirectionalLightComponent>()->SetIntensity(4.0f);

    FViewportCamera& Camera = *GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetPerspectiveCamera();
    CameraLocation = Camera.GetLocation();
    CameraRotation = Camera.GetRotation();
    
    Camera.SetRotation(FVector(0.0f, 30, 180));
    if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(MeshComp))
    {
        float FOV = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraFOV();

        // 로컬 바운딩 박스
        FBoundingBox Box = Primitive->GetBoundingBox();
        FVector LocalCenter = (Box.MinLocation + Box.MaxLocation) * 0.5f;
        FVector LocalExtents = (Box.MaxLocation - Box.MinLocation) * 0.5f;
        float Radius = LocalExtents.Length();
        
        FMatrix ComponentToWorld = Primitive->GetWorldMatrix();
        FVector WorldCenter = ComponentToWorld.TransformPosition(LocalCenter);

        // FOV 기반 거리 계산
        float VerticalFOV = FMath::DegreesToRadians(FOV);
        float Distance = Radius / FMath::Tan(VerticalFOV * 0.5f);

        // 카메라 위치 설정
        Camera.SetLocation(WorldCenter - Camera.GetForwardVector() * Distance);
    }

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_LOCAL);
    }
}

void UEditorEngine::StartParticleViewer(UParticleSystem* ParticleSystemAsset)
{
    if (!ParticleSystemAsset)
    {
        return;
    }

    ViewerType = EViewerType::EVT_ParticleViewer;

    ClearActorSelection();
    ClearComponentSelection();
    
    if (ParticleViewerWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("SkeletalMeshViewerWorld already exists!"));
        const auto Actors = ParticleViewerWorld->GetActiveLevel()->Actors;
        for (const auto& Actor : Actors)
        {
            Actor->Destroy();
        }
    }
    else
    {
        FWorldContext& WorldContext = CreateNewWorldContext(EWorldType::ParticleViewer);
        ParticleViewerWorld = UParticleViewerWorld::CreateWorld(this);
        WorldContext.SetCurrentWorld(ParticleViewerWorld);
    }
    
    ActiveWorld = ParticleViewerWorld;
    ParticleViewerWorld->WorldType = EWorldType::ParticleViewer;
    
    // 파티클 스폰
    AActor* ParticleActor = ParticleViewerWorld->SpawnActor<AActor>();
    ParticleActor->SetActorTickInEditor(true);
    
    UParticleSystemComponent* ParticleSystemComponent = ParticleActor->AddComponent<UParticleSystemComponent>();
    ParticleSystemComponent->SetParticleSystem(ParticleSystemAsset);
    
    ParticleActor->SetRootComponent(ParticleSystemComponent);
    ParticleActor->SetActorLabel(TEXT("OBJ_PARTICLE"));

    // World와 Panel에 ParticleSystem 설정
    ParticleViewerWorld->SetParticleSystem(ParticleSystemComponent);
    auto EditorPanel = GEngineLoop.GetUnrealEditor()->GetEditorPanel("ParticleViewerPanel");
    auto ParticlePanel = std::dynamic_pointer_cast<ParticleViewerPanel>(EditorPanel);
    ParticleViewerPanel* ParticleViewerPanel = ParticlePanel.get();
    ParticleViewerPanel->SetParticleSystemComponent(ParticleSystemComponent);
    ParticleViewerPanel->SetParticleSystem(ParticleSystemAsset);

    // ParticleActor 강제 설정
    Cast<UEditorEngine>(GEngine)->SelectActor(ParticleActor);
    Cast<UEditorEngine>(GEngine)->SelectComponent(ParticleSystemComponent);

    FViewportCamera& Camera = *GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetPerspectiveCamera();

    CameraLocation = Camera.GetLocation();
    CameraRotation = Camera.GetRotation();
    
    FVector NewCameraLocation = FVector(8, 8 , 8);

    FVector Delta = (FVector(0.f, 0.f, 5.f) - NewCameraLocation).GetSafeNormal();
    float Pitch = FMath::RadiansToDegrees(FMath::Asin(Delta.Z));
    float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    FVector NewCameraRotation = FVector(0, -Pitch,  Yaw);
    
    Camera.SetLocation(NewCameraLocation);
    Camera.SetRotation(NewCameraRotation);

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_LOCAL);
    }

    ClearActorSelection();
    ClearComponentSelection();
}

void UEditorEngine::StartPhysicsAssetViewer(FName PreviewMeshKey, FName PhysicsAssetName)
{
    if (PreviewMeshKey == "")
    {
        return;
    }

    ViewerType = EViewerType::EVT_PhysicsAssetViewer;

    ClearActorSelection();
    ClearComponentSelection();

    if (PhysicsAssetViewerWorld)
    {
        UE_LOG(ELogLevel::Warning, TEXT("PhysicsAssetViewerWorld already exists!"));
        const auto Actors = PhysicsAssetViewerWorld->GetActiveLevel()->Actors;
        for (const auto& Actor : Actors)
        {
            Actor->Destroy();
        }
    }
    else
    {
        FWorldContext& WorldContext = CreateNewWorldContext(EWorldType::PhysicsAssetViewer);
        PhysicsAssetViewerWorld = UPhysicsAssetViewerWorld::CreateWorld(this);
        WorldContext.SetCurrentWorld(PhysicsAssetViewerWorld);
    }

    ActiveWorld = PhysicsAssetViewerWorld;
    PhysicsAssetViewerWorld->WorldType = EWorldType::PhysicsAssetViewer;
    PhysicsManager->CreateScene(ActiveWorld);
    PhysicsManager->SetCurrentScene(ActiveWorld);

    // 스켈레탈 액터 스폰
    ASkeletalMeshActor* SkeletalActor = PhysicsAssetViewerWorld->SpawnActor<ASkeletalMeshActor>();
    SkeletalActor->SetActorTickInEditor(true);

    USkeletalMeshComponent* MeshComp = SkeletalActor->AddComponent<USkeletalMeshComponent>();
    SkeletalActor->SetRootComponent(MeshComp);
    SkeletalActor->SetActorLabel(TEXT("OBJ_SKELETALMESH"));

    USkeletalMesh* PreviewMesh = Cast<USkeletalMesh>(UAssetManager::Get().GetAsset(EAssetType::SkeletalMesh, PreviewMeshKey.ToString()));
    MeshComp->SetSkeletalMeshAsset(PreviewMesh);

    // Set Physics Asset
    UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(UAssetManager::Get().GetAsset(EAssetType::PhysicsAsset, PhysicsAssetName.ToString()));
    if (!PhysicsAsset)
    {
        PhysicsAsset = FObjectFactory::ConstructObject<UPhysicsAsset>(nullptr);
        PhysicsAsset->SetPreviewMesh(PreviewMesh);
        
        FAssetInfo Info;
        Info.AssetName = PhysicsAsset->GetName();
        Info.PackagePath = TEXT("Contents/PhysicsAsset");
        Info.AssetType = EAssetType::PhysicsAsset;
        Info.AssetObject = PhysicsAsset;
        UAssetManager::Get().AddAssetInfo(Info);

        UAssetManager::Get().AddAsset(Info.GetFullPath(), PhysicsAsset);
    }
    PreviewMesh->SetPhysicsAsset(PhysicsAsset);
    
    if (PhysicsAssetViewerWorld) // PhysicsAssetViewerWorld가 유효한지 다시 확인하는 것이 좋습니다.
    {
        PhysicsAssetViewerWorld->SetSkeletalMeshComponent(MeshComp);
    }
    else
    {
        UE_LOG(ELogLevel::Error, TEXT("PhysicsAssetViewerWorld is null after creation attempt in StartPhysicsAssetViewer."));
    }

    ADirectionalLight* DirectionalLight = PhysicsAssetViewerWorld->SpawnActor<ADirectionalLight>();
    DirectionalLight->SetActorRotation(FRotator(45.f, 45.f, 0.f));
    DirectionalLight->GetComponentByClass<UDirectionalLightComponent>()->SetIntensity(4.0f);

    FViewportCamera& Camera = *GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetPerspectiveCamera();
    CameraLocation = Camera.GetLocation();
    CameraRotation = Camera.GetRotation();

    Camera.SetRotation(FVector(0.0f, 30, 180));
    if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(MeshComp))
    {
        float FOV = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraFOV();

        // 로컬 바운딩 박스
        FBoundingBox Box = Primitive->GetBoundingBox();
        FVector LocalCenter = (Box.MinLocation + Box.MaxLocation) * 0.5f;
        FVector LocalExtents = (Box.MaxLocation - Box.MinLocation) * 0.5f;
        float Radius = LocalExtents.Length();

        FMatrix ComponentToWorld = Primitive->GetWorldMatrix();
        FVector WorldCenter = ComponentToWorld.TransformPosition(LocalCenter);

        // FOV 기반 거리 계산
        float VerticalFOV = FMath::DegreesToRadians(FOV);
        float Distance = Radius / FMath::Tan(VerticalFOV * 0.5f);

        // 카메라 위치 설정
        Camera.SetLocation(WorldCenter - Camera.GetForwardVector() * Distance);
    }

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_LOCAL);
    }
}

void UEditorEngine::BindEssentialObjects()
{
    for (const auto Iter: TObjectRange<APlayer>())
    {
        if (Iter->GetWorld() == ActiveWorld)
        {
            ActiveWorld->SetMainPlayer(Iter);
            break;
        }
    }
    
    //실수로 안만들면 넣어주기
    if (ActiveWorld->GetMainPlayer() == nullptr)
    {
        APlayer* TempPlayer = ActiveWorld->SpawnActor<APlayer>();
        TempPlayer->SetActorLabel(TEXT("OBJ_PLAYER"));
        TempPlayer->SetActorTickInEditor(false);
        ActiveWorld->SetMainPlayer(TempPlayer);
    }
    
    //무조건 PIE들어갈때 만들어주기
    APlayerController* PlayerController = ActiveWorld->SpawnActor<APlayerController>();
    PlayerController->SetActorLabel(TEXT("OBJ_PLAYER_CONTROLLER"));
    PlayerController->SetActorTickInEditor(false);
    ActiveWorld->SetPlayerController(PlayerController);
    
    ActiveWorld->GetPlayerController()->Possess(ActiveWorld->GetMainPlayer());
}

void UEditorEngine::SetPhysXScene(UWorld* World)
{
    PhysicsManager->CreateScene(PIEWorld);
    PhysicsManager->SetCurrentScene(PIEWorld);

    for (const auto& Actor : World->GetActiveLevel()->Actors)
    {
        UPrimitiveComponent* Prim = Actor->GetComponentByClass<UPrimitiveComponent>();
        if (Prim && Prim->bSimulate)
        {
            Prim->CreatePhysXGameObject();
        }
    }
}

void UEditorEngine::EndPIE()
{
    ViewerType = EViewerType::EVT_Editor;
    
    if (PIEWorld)
    {
        this->ClearActorSelection(); // PIE World 기준 Select Actor 해제 
        WorldList.Remove(GetWorldContextFromWorld(PIEWorld));
        PIEWorld->Release();
        GUObjectArray.MarkRemoveObject(PIEWorld);
        PIEWorld = nullptr;

        // TODO: PIE에서 EditorWorld로 돌아올 때, 기존 선택된 Picking이 유지되어야 함. 현재는 에러를 막기위해 임시조치.
        ClearActorSelection();
        ClearComponentSelection();
        PhysicsManager->CleanupScene();
    }

    FSlateAppMessageHandler* Handler = GEngineLoop.GetAppMessageHandler();

    Handler->OnPIEModeEnd();
    // 다시 EditorWorld로 돌아옴.
    ActiveWorld = EditorWorld;
}

void UEditorEngine::EndSkeletalMeshViewer()
{
    ViewerType = EViewerType::EVT_Editor;
    
    if (SkeletalMeshViewerWorld)
    {
        this->ClearActorSelection();
        WorldList.Remove(GetWorldContextFromWorld(SkeletalMeshViewerWorld));
        SkeletalMeshViewerWorld->Release();
        GUObjectArray.MarkRemoveObject(SkeletalMeshViewerWorld);
        SkeletalMeshViewerWorld = nullptr;
        
        FViewportCamera& Camera = *GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetPerspectiveCamera();
        Camera.SetLocation(CameraLocation);
        Camera.SetRotation(CameraRotation);
        
        ClearActorSelection();
        ClearComponentSelection();
    }
    ActiveWorld = EditorWorld;

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_WORLD);
    }
}

void UEditorEngine::EndParticleViewer()
{
    ViewerType = EViewerType::EVT_Editor;
    
    if (ParticleViewerWorld)
    {
        this->ClearActorSelection();
        WorldList.Remove(GetWorldContextFromWorld(ParticleViewerWorld));
        ParticleViewerWorld->Release();
        GUObjectArray.MarkRemoveObject(ParticleViewerWorld);
        ParticleViewerWorld = nullptr;
        
        FViewportCamera& Camera = *GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetPerspectiveCamera();
        Camera.SetLocation(CameraLocation);
        Camera.SetRotation(CameraRotation);
        
        ClearActorSelection();
        ClearComponentSelection();
    }
    ActiveWorld = EditorWorld;

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_WORLD);
    }
}

void UEditorEngine::EndPhysicsAssetViewer()
{
    ViewerType = EViewerType::EVT_Editor;
    
    if (PhysicsAssetViewerWorld)
    {
        this->ClearActorSelection();
        WorldList.Remove(GetWorldContextFromWorld(PhysicsAssetViewerWorld));
        PhysicsAssetViewerWorld->Release();
        GUObjectArray.MarkRemoveObject(PhysicsAssetViewerWorld);
        PhysicsAssetViewerWorld = nullptr;

        FViewportCamera& Camera = *GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetPerspectiveCamera();
        Camera.SetLocation(CameraLocation);
        Camera.SetRotation(CameraRotation);

        ClearActorSelection();
        ClearComponentSelection();
    }
    ActiveWorld = EditorWorld;

    if (AEditorPlayer* Player = GetEditorPlayer())
    {
        Player->SetCoordMode(ECoordMode::CDM_WORLD);
    }
}

FWorldContext& UEditorEngine::GetEditorWorldContext(/*bool bEnsureIsGWorld*/)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->WorldType == EWorldType::Editor)
        {
            return *WorldContext;
        }
    }
    return CreateNewWorldContext(EWorldType::Editor);
}

FWorldContext* UEditorEngine::GetPIEWorldContext(/*int32 WorldPIEInstance*/)
{
    for (FWorldContext* WorldContext : WorldList)
    {
        if (WorldContext->WorldType == EWorldType::PIE)
        {
            return WorldContext;
        }
    }
    return nullptr;
}

void UEditorEngine::SelectActor(AActor* InActor)
{
    if (InActor && CanSelectActor(InActor))
    {
        UE_LOGFMT(ELogLevel::Display, "Select Actor: {}", InActor->GetName());
        PrivateEditorSelection::GActorSelected = InActor;
    }
}

void UEditorEngine::DeselectActor(AActor* InActor)
{
    if (PrivateEditorSelection::GActorSelected == InActor && InActor)
    {
        UE_LOGFMT(ELogLevel::Display, "Deselect Actor: {}", InActor->GetName());
        PrivateEditorSelection::GActorSelected = nullptr;
        ClearComponentSelection();
    }
}

void UEditorEngine::ClearActorSelection()
{
    UE_LOGFMT(ELogLevel::Display, "Clear Actor Selection");
    PrivateEditorSelection::GActorSelected = nullptr;
}

bool UEditorEngine::CanSelectActor(const AActor* InActor) const
{
    return InActor != nullptr && InActor->GetWorld() == ActiveWorld && !InActor->IsActorBeingDestroyed();
}

AActor* UEditorEngine::GetSelectedActor() const
{
    return PrivateEditorSelection::GActorSelected;
}

void UEditorEngine::HoverActor(AActor* InActor)
{
    if (InActor)
    {
        PrivateEditorSelection::GActorHovered = InActor;
    }
}

void UEditorEngine::NewLevel()
{
    ClearActorSelection();
    ClearComponentSelection();

    if (ActiveWorld->GetActiveLevel())
    {
        ActiveWorld->GetActiveLevel()->Release();
    }
}

void UEditorEngine::SelectComponent(USceneComponent* InComponent) const
{
    if (InComponent && CanSelectComponent(InComponent))
    {
        UE_LOGFMT(ELogLevel::Display, "Select Component: {}", InComponent->GetName());
        PrivateEditorSelection::GComponentSelected = InComponent;
    }
}

void UEditorEngine::DeselectComponent(USceneComponent* InComponent)
{
    // 전달된 InComponent가 현재 선택된 컴포넌트와 같다면 선택 해제
    if (PrivateEditorSelection::GComponentSelected == InComponent && InComponent != nullptr)
    {
        UE_LOGFMT(ELogLevel::Display, "Deselect Component: {}", InComponent->GetName());
        PrivateEditorSelection::GComponentSelected = nullptr;
    }
}

void UEditorEngine::ClearComponentSelection()
{
    UE_LOGFMT(ELogLevel::Display, "Clear Component Selection");
    PrivateEditorSelection::GComponentSelected = nullptr;
}

bool UEditorEngine::CanSelectComponent(const USceneComponent* InComponent) const
{
    return InComponent != nullptr && InComponent->GetOwner() && InComponent->GetOwner()->GetWorld() == ActiveWorld && !InComponent->GetOwner()->IsActorBeingDestroyed();
}

USceneComponent* UEditorEngine::GetSelectedComponent() const
{
    return PrivateEditorSelection::GComponentSelected;
}

void UEditorEngine::HoverComponent(USceneComponent* InComponent)
{
    if (InComponent)
    {
        PrivateEditorSelection::GComponentHovered = InComponent;
    }
}

AEditorPlayer* UEditorEngine::GetEditorPlayer() const
{
    return EditorPlayer;
}
