#include "SceneManager.h"
#include <fstream>
#include "EditorViewportClient.h"
#include "Engine/FObjLoader.h"
#include "Engine/StaticMeshActor.h"
#include "UObject/Casts.h"
#include "UObject/Object.h"
#include "UObject/ObjectFactory.h"
#include "UObject/ObjectGlobals.h"

#include "JSON/json.hpp"
#include "World/World.h"

using namespace NS_SceneManagerData;
using json = nlohmann::json;


#pragma region nlohmann::json function overload
[[maybe_unused]]
static void to_json(json& Json, const FString& S)
{
    Json = S.GetContainerPrivate();
}

[[maybe_unused]]
static void from_json(const json& Json, FString& S)
{
    if (Json.is_string())
    {
        Json.get_to(S.GetContainerPrivate());
    }
}


template <typename ElementType, typename AllocatorType>
[[maybe_unused]]
static void to_json(json& Json, const TArray<ElementType, AllocatorType>& Array)
{
    Json = Array.GetContainerPrivate();
}

template <typename ElementType, typename AllocatorType>
[[maybe_unused]]
static void from_json(const json& Json, TArray<ElementType, AllocatorType>& Array)
{
    Json.get_to(Array.GetContainerPrivate());
}


template <typename KeyType, typename ValueType, typename Allocator>
[[maybe_unused]]
static void to_json(json& Json, const TMap<KeyType, ValueType, Allocator>& Map)
{
    Json = Map.GetContainerPrivate();
}

template <typename KeyType, typename ValueType, typename Allocator>
[[maybe_unused]]
static void from_json(const json& Json, TMap<KeyType, ValueType, Allocator>& Map)
{
    Json.get_to(Map.GetContainerPrivate());
}
#pragma endregion

namespace NS_SceneManagerData
{
// 컴포넌트 하나의 저장 정보를 담는 구조체
struct FComponentSaveData
{
    FString ComponentID;    // 컴포넌트의 고유 ID (액터 내에서 유일해야 함, FName) 
    FString ComponentClass; // 컴포넌트 클래스 이름 (예: "UStaticMeshComponent", "UPointLightComponent")

    TMap<FString, FString> Properties;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FComponentSaveData, ComponentID, ComponentClass, Properties)
};

// 액터 하나의 저장 정보를 담는 구조체
struct FActorSaveData
{
    FString ActorID;    // 액터의 고유 ID FName
    FString ActorClass; // 액터의 클래스 이름 (예: "AStaticMeshActor", "APointLight")
    FString ActorLabel; // 에디터에서 보이는 이름 (선택적)
    // FTransform ActorTransform; // 액터 자체의 트랜스폼 (보통 루트 컴포넌트가 결정) - 필요 여부 검토
    FString ActorTickInEditor;

    FString RootComponentID;               // 이 액터의 루트 컴포넌트 ID (아래 Components 리스트 내 ID 참조)
    TArray<FComponentSaveData> Components; // 이 액터가 소유한 컴포넌트 목록

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FActorSaveData, ActorID, ActorClass, ActorLabel, ActorTickInEditor, RootComponentID, Components)
};

struct FSceneData
{
    int32 Version = 0;
    int32 NextUUID = 0;
    //TMap<int32, UObject*> Primitives;

    TArray<FActorSaveData> Actors; // 씬에 있는 모든 액터 정보
    //TMap<int32, UObject*> Cameras;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(FSceneData, Version, NextUUID, Actors)
};

//TODO : 레벨 데이타 구현
}


void SceneManager::LoadSceneFromJsonFile(const std::filesystem::path& FilePath, UWorld& OutWorld)
{
    std::ifstream JsonFile(FilePath);
    if (!JsonFile.is_open())
    {
        UE_LOG(ELogLevel::Error, "Failed to open file for reading: %s", FilePath.c_str());
        return;
    }

    FString JsonString;
    JsonFile.seekg(0, std::ios::end);
    const int64 Size = JsonFile.tellg();
    JsonString.Resize(static_cast<int32>(Size));

    JsonFile.seekg(0, std::ios::beg);
    JsonFile.read(&JsonString[0], Size);
    JsonFile.close();

    FSceneData SceneData;
    bool Result = JsonToSceneData(JsonString,SceneData);
    if (!Result)
    {
        UE_LOG(ELogLevel::Error, "Failed to parse scene data from file: %s", FilePath.c_str());
        return ;
    }

    LoadWorldFromData(SceneData, &OutWorld);
}

bool SceneManager::SaveSceneToJsonFile(const std::filesystem::path& FilePath, const UWorld& InWorld)
{
    FSceneData SceneData = WorldToSceneData(InWorld);

    std::filesystem::path ParentPath = FilePath.parent_path();
    if (!std::filesystem::exists(ParentPath))
    {
        try
        {
            std::filesystem::create_directory(ParentPath);
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            UE_LOG(ELogLevel::Error, TEXT("Failed to create directory: %s"), *FString(e.what()));
            return false;
        }
    }
    
    std::ofstream outFile(FilePath);
    if (!outFile)
    {
        MessageBoxA(nullptr, "Failed to open file for writing: ", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    FString JsonData;
    SceneDataToJson(SceneData, JsonData);
    outFile << JsonData.GetContainerPrivate();
    outFile.close();

    return true;
}

bool SceneManager::JsonToSceneData(const FString& InJsonString, FSceneData& OutSceneData)
{
    try
    {
        const json Json = json::parse(InJsonString.GetContainerPrivate()); // JSON 파일 읽기
        OutSceneData = Json;
    }
    catch (const std::exception& e)
    {
        UE_LOG(ELogLevel::Error, "Error parsing JSON: %s", e.what());
        return false;
    }
    return true;
}

bool SceneManager::SceneDataToJson(const FSceneData& InSceneData, FString& OutJsonString)
{
    try
    {
        const json Json = InSceneData;
        OutJsonString = Json.dump(4); // JSON 데이터를 문자열로 변솬 (4는 들여쓰기 공백 수)
    }
    catch (const std::exception& e)
    {
        UE_LOG(ELogLevel::Error, "Error parsing JSON: %s", e.what());
        return false;
    }
    return true;
}

FSceneData SceneManager::WorldToSceneData(const UWorld& InWorld)
{
    FSceneData sceneData;
    sceneData.Version = 1;

    const TArray<AActor*>& Actors =  InWorld.GetActiveLevel()->Actors;

    sceneData.Actors.Reserve(Actors.Num());

    for (const auto& Actor : Actors)
    {
        FActorSaveData actorData;

        actorData.ActorID = Actor->GetName();
        actorData.ActorClass = Actor->GetClass()->GetName();
        actorData.ActorLabel = Actor->GetActorLabel();
        actorData.ActorTickInEditor = Actor->IsActorTickInEditor() ? "true" : "false";

        USceneComponent* RootComp = Actor->GetRootComponent();
        actorData.RootComponentID = (RootComp != nullptr) ? RootComp->GetName() : TEXT(""); // 루트 없으면 빈 문자열
        
        for (const auto& Component : Actor->GetComponents())
        {
            FComponentSaveData componentData;
            componentData.ComponentID = Component->GetName();
            componentData.ComponentClass = Component->GetClass()->GetName();
            
            //TMap<FString, FString> InProperties;
            Component->GetProperties(componentData.Properties);
            
            // 컴포넌트의 속성들을 JSON으로 변환하여 저장
            // for (const auto& Property : InProperties)
            // {
            //    FString Value = Property.Value;
            //     
            //     componentData.Properties[Property.Key] = Value;
            // }

            actorData.Components.Add(componentData);
        }
        sceneData.Actors.Add(actorData);
    }
    return sceneData;
}

bool SceneManager::LoadWorldFromData(const FSceneData& sceneData, UWorld* targetWorld)
{
    if (targetWorld == nullptr)
    {
        UE_LOG(ELogLevel::Error, TEXT("LoadSceneFromData: Target World is null!"));
        return false;
    }

    // 임시 맵: 저장된 ID와 새로 생성된 객체 포인터를 매핑
    TMap<FString, AActor*> SpawnedActorsMap;
    //TMap<FString, UActorComponent*> SpawnedComponentsMap;
    

    // --- 1단계: 액터 및 컴포넌트 생성 ---
    UE_LOG(ELogLevel::Display, TEXT("Loading Scene Data: Phase 1 - Spawning Actors and Components..."));
    for (const FActorSaveData& actorData : sceneData.Actors)
    {
        // 1.1. 액터 클래스 찾기
        
        UClass* classAActor = UClass::FindClass(FName(actorData.ActorClass));
        
        AActor* SpawnedActor = targetWorld->SpawnActor(classAActor, FName(actorData.ActorID));

        // if (actorData.ActorClass == AActor::StaticClass()->GetName())
        // {
        //     SpawnedActor = targetWorld->SpawnActor<AActor>();
        // }
        // if (actorData.ActorClass == AStaticMeshActor::StaticClass()->GetName())
        // {
        //     SpawnedActor = targetWorld->SpawnActor<AStaticMeshActor>();
        // }
        // // 또는 특정 경로에서 클래스 로드: UClass* ActorClass = LoadClass<AActor>(nullptr, *actorData.ActorClass);
        if (SpawnedActor == nullptr)
        {
            UE_LOG(ELogLevel::Error, TEXT("LoadSceneFromData: Could not find Actor Class '%s'. Skipping Actor '%s'."),
                   *actorData.ActorClass, *actorData.ActorID);
            continue;
        }

        
        // 액터 클래스가 AActor의 자식인지 확인

        // 1.2. 액터 스폰 (기본 위치/회전 사용, 나중에 루트 컴포넌트가 설정)
        //FActorSpawnParameters SpawnParams;
        //SpawnParams.Name = FName(*actorData.ActorID); // 저장된 ID를 이름으로 사용 시도 (Unique해야 함)
        //SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested; // 이름 충돌 시 엔진이 처리하도록 할 수도 있음
        //AActor* SpawnedActor = targetWorld->SpawnActor<AActor>(ActorClass, FVector::ZeroVector);

        if (SpawnedActor == nullptr)
        {
            UE_LOG(ELogLevel::Error, TEXT("LoadSceneFromData: Failed to spawn Actor '%s' of class '%s'."),
                   *actorData.ActorID, *actorData.ActorClass);
            continue;
        }

        SpawnedActor->SetActorLabel(actorData.ActorLabel, false); // 액터 레이블 설정
        SpawnedActor->SetActorTickInEditor(actorData.ActorTickInEditor == "true");
        SpawnedActorsMap.Add(actorData.ActorID, SpawnedActor); // 맵에 추가

        // 액터별 로컬 컴포넌트 맵: ComponentID -> 생성/재사용된 컴포넌트 포인터
        TMap<FString, UActorComponent*> ActorComponentsMap;

        // 1.3. 컴포넌트 생성 및 속성 설정 (아직 부착 안 함)
        for (const FComponentSaveData& componentData : actorData.Components)
        {
            UClass* ComponentClass =  UClass::FindClass(FName(componentData.ComponentClass));


            // 컴포넌트 생성 (액터를 Outer로 지정, 저장된 ID를 이름으로)
            UActorComponent* TargetComponent = nullptr; // 최종적으로 사용할 컴포넌트 포인터

            // *** 핵심 변경: 저장된 ID(이름)로 액터에서 기존 컴포넌트를 먼저 찾아본다 ***
            FName ComponentFName(*componentData.ComponentID);
            TargetComponent = FindObject<UActorComponent>(SpawnedActor, ComponentFName); // Outer를 SpawnedActor로 지정하여 검색

            // 클래스 일치 확인
            if (TargetComponent && TargetComponent->GetClass()->GetName() != componentData.ComponentClass) {
                UE_LOG(ELogLevel::Warning, TEXT("Component '%s' class mismatch. Recreating."), *componentData.ComponentID);
                // TODO: 기존 컴포넌트를 제거해야 할 수도 있음? 아니면 그냥 새것으로 덮어쓰나? 정책 필요.
                TargetComponent = nullptr; // 새로 생성하도록 리셋
            }

            // 기존 컴포넌트가 없으면 새로 생성
            if (TargetComponent == nullptr)
            {
                TargetComponent = SpawnedActor->AddComponent(ComponentClass, FName(componentData.ComponentID), false);
                
                // if (!actorData.RootComponentID.IsEmpty())
                // {
                //     if (componentData.ComponentID != actorData.RootComponentID)
                //     {
                //         // 임시로 RootComponent 가 아니면 떼어줌
                //         USceneComponent* SceneComp = Cast<USceneComponent>(TargetComponent);
                //         if (SceneComp)
                //         {
                //             SpawnedActor->SetRootComponent(nullptr);
                //         }
                //     }
                // }
                
                // if (componentData.ComponentClass == UStaticMesh::StaticClass()->GetName())
                // {
                //     TargetComponent = SpawnedActor->AddComponent<UStaticMeshComponent>();
                // }
                // else if (componentData.ComponentClass == UCubeComp::StaticClass()->GetName())
                // {
                //     TargetComponent = SpawnedActor->AddComponent<UCubeComp>();
                // }
                // else
                // {
                //     TargetComponent = SpawnedActor->AddComponent<UActorComponent>();
                // }
                
                // !!! 중요: 컴포넌트 등록 !!!
                //NewComponent->RegisterComponent();
            }
            
            if (TargetComponent == nullptr)
            {
                 UE_LOG(ELogLevel::Error, TEXT("LoadSceneFromData: Failed to create Component '%s' of class '%s' for Actor '%s'."),
                       *componentData.ComponentID, *componentData.ComponentClass, *actorData.ActorID);
                continue;
            }

            // --- 이제 TargetComponent는 유효한 기존 컴포넌트 또는 새로 생성된 컴포넌트 ---
            if (TargetComponent)
            {
                // 1.4. 컴포넌트 속성 설정 (공통 로직)
                //ApplyComponentProperties(TargetComponent, componentData.Properties);
                TargetComponent->SetProperties( componentData.Properties); // 태그 설정 (ID로 사용)

                // 1.5. *** 수정: 복합 키를 사용하여 컴포넌트 맵에 추가 ***
                //FString CompositeKey = actorData.ActorID + TEXT("::") + componentData.ComponentID; // 예: "MyActor1::MeshComponent"
                ActorComponentsMap.Add(componentData.ComponentID, TargetComponent);
            }
        }

        // 루트 컴포넌트 설정
        if (!actorData.RootComponentID.IsEmpty())
        {
            UActorComponent** FoundRootCompPtr = ActorComponentsMap.Find(actorData.RootComponentID);
            if (FoundRootCompPtr && *FoundRootCompPtr)
            {
                USceneComponent* RootSceneComp = Cast<USceneComponent>(*FoundRootCompPtr);
                if (RootSceneComp) {
                    SpawnedActor->SetRootComponent(RootSceneComp);
                    UE_LOG(ELogLevel::Display, TEXT("Set RootComponent '%s' for Actor '%s'"), *actorData.RootComponentID, *actorData.ActorID);
                }
                else { /* 루트가 SceneComponent 아님 경고 */ }
            }
            else { /* 루트 컴포넌트 못 찾음 경고 */ }
        }

        // 컴포넌트 부착 및 상대 트랜스폼 설정
        for (const FComponentSaveData& componentData : actorData.Components) // 다시 컴포넌트 데이터 순회
        {
            UActorComponent** FoundCompPtr = ActorComponentsMap.Find(componentData.ComponentID);
            if (FoundCompPtr == nullptr || *FoundCompPtr == nullptr) continue; // 위에서 생성/찾기 실패한 경우

            USceneComponent* CurrentSceneComp = Cast<USceneComponent>(*FoundCompPtr);
            if (CurrentSceneComp == nullptr) continue; // SceneComponent만 부착/트랜스폼 가능

            // 부착 정보 찾기 (Properties 맵 사용)
            const FString* ParentIDPtr = componentData.Properties.Find(TEXT("AttachParentID"));
            if (ParentIDPtr && !ParentIDPtr->IsEmpty() && *ParentIDPtr != TEXT("nullptr"))
            {
                // !!! 부모 검색 범위를 ActorComponentsMap (현재 액터의 컴포넌트)으로 한정 !!!
                UActorComponent** FoundParentCompPtr = ActorComponentsMap.Find(*ParentIDPtr);
                if (FoundParentCompPtr && *FoundParentCompPtr)
                {
                    USceneComponent* ParentSceneComp = Cast<USceneComponent>(*FoundParentCompPtr);
                    if (ParentSceneComp) {
                        // 부착 실행 (SetupAttachment 대신 AttachToComponent 권장 - 규칙 명시 가능)
                        CurrentSceneComp->SetupAttachment(ParentSceneComp);
                        UE_LOG(ELogLevel::Display, TEXT("Attached Component '%s' to Parent '%s' in Actor '%s'"), *componentData.ComponentID, *(*ParentIDPtr), *actorData.ActorID);
                    }
                    else { /* 부모가 SceneComponent 아님 경고 */ }
                }
                else {
                    // 부모 컴포넌트를 이 액터 내에서 찾지 못함 (오류 가능성 높음)
                    UE_LOG(ELogLevel::Warning, TEXT("Could not find Parent component '%s' within Actor '%s' for '%s'."), *(*ParentIDPtr), *actorData.ActorID, *componentData.ComponentID);
                }
            }

            FVector RelativeLocation = FVector::ZeroVector;
            const FString* LocStr = componentData.Properties.Find(TEXT("RelativeLocation"));
            if (LocStr) RelativeLocation.InitFromString(*LocStr); // 또는 직접 파싱

            FRotator RelativeRotation;
            const FString* RotatStr = componentData.Properties.Find(TEXT("RelativeRotation")); // 쿼터니언 저장/로드 권장
            if (RotatStr) RelativeRotation.InitFromString(*RotatStr);

            FVector RelativeScale3D = FVector::OneVector;
            const FString* ScaleStr = componentData.Properties.Find(TEXT("RelativeScale3D")); // 스케일 키 이름 확인! (GetProperties와 일치해야 함)
            if (ScaleStr) RelativeScale3D.InitFromString(*ScaleStr);

            CurrentSceneComp->SetRelativeLocation(RelativeLocation);
            CurrentSceneComp->SetRelativeRotation(RelativeRotation);
            CurrentSceneComp->SetRelativeScale3D(RelativeScale3D);
        }

    }
    UE_LOG(ELogLevel::Display, TEXT("Loading Scene Data: Phase 1 Complete. Spawned %d actors."), SpawnedActorsMap.Num());

    UE_LOG(ELogLevel::Display, TEXT("Scene loading complete."));

    // 임시 맵 정리 (선택적)
    SpawnedActorsMap.Empty();
    //SpawnedComponentsMap.Empty();

    // 필요하다면 추가적인 월드 초기화 로직 (예: 네비게이션 재빌드 요청)
    // ...

    UE_LOG(ELogLevel::Display, TEXT("Scene loading complete."));
    return true;
}
