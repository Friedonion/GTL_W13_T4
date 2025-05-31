#include "EditorRenderPass.h"

#include "EngineLoop.h" // GEngineLoop
#include "Engine/Source/Runtime/Engine/Classes/Engine/Engine.h" // GEngine
#include "Engine/Source/Runtime/CoreUObject/UObject/Casts.h"
#include "Engine/Source/Runtime/Engine/Classes/Engine/EditorEngine.h"
#include <D3D11RHI/DXDShaderManager.h>

#include "UnrealClient.h"
#include "Engine/Source/Runtime/Engine/World/World.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"
#include "BaseGizmos/GizmoBaseComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/FObjLoader.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Classes/Actors/Player.h"
#include "Engine/Classes/Components/Light/LightComponent.h"
#include "Engine/Classes/Components/Light/DirectionalLightComponent.h"
#include "Engine/Classes/Components/Light/SpotLightComponent.h"
#include "Engine/Classes/Components/Light/PointLightComponent.h"
#include "Engine/Classes/Components/HeightFogComponent.h"
#include "Math/JungleMath.h"
#include "PropertyEditor/ShowFlags.h"

void FEditorRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);
    
    CreateShaders();
    CreateBuffers();
    CreateConstantBuffers();
}

void FEditorRenderPass::CreateShaders()
{
    D3D11_INPUT_ELEMENT_DESC LayoutGizmo[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3D11_INPUT_ELEMENT_DESC LayoutPosOnly[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    auto AddShaderSet = [this](const std::wstring& KeyPrefix, const std::string& VsEntry, const std::string& PsEntry, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize)
        {
            ShaderManager->AddVertexShaderAndInputLayout(KeyPrefix + L"VS", L"Shaders/EditorShader.hlsl", VsEntry, Layout, LayoutSize);
            ShaderManager->AddPixelShader(KeyPrefix + L"PS", L"Shaders/EditorShader.hlsl", PsEntry);
        };

    auto AddShaderSetWithoutLayout = [this](const std::wstring& KeyPrefix, const std::string& VsEntry, const std::string& PsEntry)
    {
        ShaderManager->AddVertexShader(KeyPrefix + L"VS", L"Shaders/EditorShader.hlsl", VsEntry);
        ShaderManager->AddPixelShader(KeyPrefix + L"PS", L"Shaders/EditorShader.hlsl", PsEntry);
    };
    

    // Cone
    AddShaderSetWithoutLayout(L"Cone", "ConeVS", "ConePS");

    // Icons
    AddShaderSetWithoutLayout(L"Icon", "IconVS", "IconPS");
    
    // Arrow (기즈모 layout 재사용)
    AddShaderSet(L"Arrow", "ArrowVS", "ArrowPS", LayoutGizmo, ARRAYSIZE(LayoutGizmo));
    
    AddShaderSet(L"Sphere", "SphereVS", "SpherePS", LayoutPosOnly, ARRAYSIZE(LayoutPosOnly));
    AddShaderSet(L"Box", "BoxVS", "BoxPS", LayoutPosOnly, ARRAYSIZE(LayoutPosOnly));
    AddShaderSet(L"Capsule", "CapsuleVS", "CapsulePS", LayoutPosOnly, ARRAYSIZE(LayoutPosOnly));
}

void FEditorRenderPass::CreateBuffers()
{
    FVertexInfo OutVertexInfo;
    FIndexInfo OutIndexInfo;
    
    ////////////////////////////////////
    // Box 버퍼 생성
    const TArray<FVector> CubeFrameVertices = {
        { -1.f, -1.f, -1.f }, // 0
        { -1.f, 1.f, -1.f }, // 1
        { 1.f, -1.f, -1.f }, // 2
        { 1.f, 1.f, -1.f }, // 3
        { -1.f, -1.f, 1.f }, // 4
        { 1.f, -1.f, 1.f }, // 5
        { -1.f, 1.f, 1.f }, // 6
        { 1.f, 1.f, 1.f }, // 7
    };

    const TArray<uint32> CubeFrameIndices = {
        // Bottom face
        0, 1, 1, 3, 3, 2, 2, 0,
        // Top face
        4, 6, 6, 7, 7, 5, 5, 4,
        // Side faces
        0, 4, 1, 6, 2, 5, 3, 7
    };


    BufferManager->CreateVertexBuffer<FVector>(TEXT("CubeVertexBuffer"), CubeFrameVertices, OutVertexInfo, D3D11_USAGE_IMMUTABLE, 0);
    BufferManager->CreateIndexBuffer<uint32>(TEXT("CubeIndexBuffer"), CubeFrameIndices, OutIndexInfo, D3D11_USAGE_IMMUTABLE, 0);

    Resources.Primitives.Box.VertexInfo = OutVertexInfo;
    Resources.Primitives.Box.IndexInfo = OutIndexInfo;
    
    ////////////////////////////////////
    // Sphere 버퍼 생성
    const TArray<FVector> SphereFrameVertices =
    {
        {1.0, 0.0, 0},
        {0.9795299412524945, 0.20129852008866006, 0},
        {0.9189578116202306, 0.39435585511331855, 0},
        {0.8207634412072763, 0.5712682150947923, 0},
        {0.6889669190756866, 0.72479278722912, 0},
        {0.5289640103269624, 0.8486442574947509, 0},
        {0.3473052528448203, 0.9377521321470804, 0},
        {0.1514277775045767, 0.9884683243281114, 0},
        {-0.05064916883871264, 0.9987165071710528, 0},
        {-0.2506525322587204, 0.9680771188662043, 0},
        {-0.4403941515576344, 0.8978045395707416, 0},
        {-0.6121059825476629, 0.7907757369376985, 0},
        {-0.758758122692791, 0.6513724827222223, 0},
        {-0.8743466161445821, 0.48530196253108104, 0},
        {-0.9541392564000488, 0.29936312297335804, 0},
        {-0.9948693233918952, 0.10116832198743228, 0},
        {-0.9948693233918952, -0.10116832198743204, 0},
        {-0.9541392564000489, -0.29936312297335776, 0},
        {-0.8743466161445822, -0.4853019625310808, 0},
        {-0.7587581226927911, -0.651372482722222, 0},
        {-0.6121059825476627, -0.7907757369376986, 0},
        {-0.44039415155763423, -0.8978045395707417, 0},
        {-0.2506525322587205, -0.9680771188662043, 0},
        {-0.05064916883871266, -0.9987165071710528, 0},
        {0.15142777750457667, -0.9884683243281114, 0},
        {0.3473052528448203, -0.9377521321470804, 0},
        {0.5289640103269624, -0.8486442574947509, 0},
        {0.6889669190756865, -0.72479278722912, 0},
        {0.8207634412072763, -0.5712682150947924, 0},
        {0.9189578116202306, -0.3943558551133187, 0},
        {0.9795299412524945, -0.20129852008866028, 0},
        {1, 0, 0},
        {1.0, 0, 0.0},
        {0.9795299412524945, 0, 0.20129852008866006},
        {0.9189578116202306, 0, 0.39435585511331855},
        {0.8207634412072763, 0, 0.5712682150947923},
        {0.6889669190756866, 0, 0.72479278722912},
        {0.5289640103269624, 0, 0.8486442574947509},
        {0.3473052528448203, 0, 0.9377521321470804},
        {0.1514277775045767, 0, 0.9884683243281114},
        {-0.05064916883871264, 0, 0.9987165071710528},
        {-0.2506525322587204, 0, 0.9680771188662043},
        {-0.4403941515576344, 0, 0.8978045395707416},
        {-0.6121059825476629, 0, 0.7907757369376985},
        {-0.758758122692791, 0, 0.6513724827222223},
        {-0.8743466161445821, 0, 0.48530196253108104},
        {-0.9541392564000488, 0, 0.29936312297335804},
        {-0.9948693233918952, 0, 0.10116832198743228},
        {-0.9948693233918952, 0, -0.10116832198743204},
        {-0.9541392564000489, 0, -0.29936312297335776},
        {-0.8743466161445822, 0, -0.4853019625310808},
        {-0.7587581226927911, 0, -0.651372482722222},
        {-0.6121059825476627, 0, -0.7907757369376986},
        {-0.44039415155763423, 0, -0.8978045395707417},
        {-0.2506525322587205, 0, -0.9680771188662043},
        {-0.05064916883871266, 0, -0.9987165071710528},
        {0.15142777750457667, 0, -0.9884683243281114},
        {0.3473052528448203, 0, -0.9377521321470804},
        {0.5289640103269624, 0, -0.8486442574947509},
        {0.6889669190756865, 0, -0.72479278722912},
        {0.8207634412072763, 0, -0.5712682150947924},
        {0.9189578116202306, 0, -0.3943558551133187},
        {0.9795299412524945, 0, -0.20129852008866028},
        {1, 0, 0},
        {0, 1.0, 0.0},
        {0, 0.9795299412524945, 0.20129852008866006},
        {0, 0.9189578116202306, 0.39435585511331855},
        {0, 0.8207634412072763, 0.5712682150947923},
        {0, 0.6889669190756866, 0.72479278722912},
        {0, 0.5289640103269624, 0.8486442574947509},
        {0, 0.3473052528448203, 0.9377521321470804},
        {0, 0.1514277775045767, 0.9884683243281114},
        {0, -0.05064916883871264, 0.9987165071710528},
        {0, -0.2506525322587204, 0.9680771188662043},
        {0, -0.4403941515576344, 0.8978045395707416},
        {0, -0.6121059825476629, 0.7907757369376985},
        {0, -0.758758122692791, 0.6513724827222223},
        {0, -0.8743466161445821, 0.48530196253108104},
        {0, -0.9541392564000488, 0.29936312297335804},
        {0, -0.9948693233918952, 0.10116832198743228},
        {0, -0.9948693233918952, -0.10116832198743204},
        {0, -0.9541392564000489, -0.29936312297335776},
        {0, -0.8743466161445822, -0.4853019625310808},
        {0, -0.7587581226927911, -0.651372482722222},
        {0, -0.6121059825476627, -0.7907757369376986},
        {0, -0.44039415155763423, -0.8978045395707417},
        {0, -0.2506525322587205, -0.9680771188662043},
        {0, -0.05064916883871266, -0.9987165071710528},
        {0, 0.15142777750457667, -0.9884683243281114},
        {0, 0.3473052528448203, -0.9377521321470804},
        {0, 0.5289640103269624, -0.8486442574947509},
        {0, 0.6889669190756865, -0.72479278722912},
        {0, 0.8207634412072763, -0.5712682150947924},
        {0, 0.9189578116202306, -0.3943558551133187},
        {0, 0.9795299412524945, -0.20129852008866028},
        {0, 1, 0}
    };

    const TArray<uint32> SphereFrameIndices =
    {
        0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10,
        11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20,
        21, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30,
        31, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40,
        41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50,
        51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 58, 58, 59, 59, 60, 60,
        61, 61, 62, 62, 63, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70,
        71, 71, 72, 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80,
        81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90, 90,
        91, 91, 92, 92, 93, 93, 94, 94, 95
    };
    
    BufferManager->CreateVertexBuffer<FVector>(TEXT("SphereVertexBuffer"), SphereFrameVertices, OutVertexInfo, D3D11_USAGE_IMMUTABLE, 0);
    BufferManager->CreateIndexBuffer<uint32>(TEXT("SphereIndexBuffer"), SphereFrameIndices, OutIndexInfo);

    Resources.Primitives.Sphere.VertexInfo = OutVertexInfo;
    Resources.Primitives.Sphere.IndexInfo = OutIndexInfo;
}

void FEditorRenderPass::CreateConstantBuffers()
{
    BufferManager->CreateBufferGeneric<FConstantBufferDebugBox>("BoxConstantBuffer", nullptr, sizeof(FConstantBufferDebugBox) * ConstantBufferSizeBox, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferDebugSphere>("SphereConstantBuffer", nullptr, sizeof(FConstantBufferDebugSphere) * ConstantBufferSizeSphere, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferDebugCapsule>("CapsuleConstantBuffer", nullptr, sizeof(FConstantBufferDebugCapsule) * ConstantBufferSizeCapsule, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferDebugCone>("ConeConstantBuffer", nullptr, sizeof(FConstantBufferDebugCone) * ConstantBufferSizeCone, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferDebugGrid>("GridConstantBuffer", nullptr, sizeof(FConstantBufferDebugGrid), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferDebugIcon>("IconConstantBuffer", nullptr, sizeof(FConstantBufferDebugIcon), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    BufferManager->CreateBufferGeneric<FConstantBufferDebugArrow>("ArrowConstantBuffer", nullptr, sizeof(FConstantBufferDebugArrow) * ConstantBufferSizeArrow, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

void FEditorRenderPass::BindRenderTarget(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    constexpr EResourceType ResourceType = EResourceType::ERT_Editor;

    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);
    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
}

void FEditorRenderPass::BindShaderResource(const std::wstring& VertexKey, const std::wstring& PixelKey, D3D_PRIMITIVE_TOPOLOGY Topology) const
{
    ID3D11VertexShader* VertexShader = ShaderManager->GetVertexShaderByKey(VertexKey);
    ID3D11PixelShader* PixelShader = ShaderManager->GetPixelShaderByKey(PixelKey);
    ID3D11InputLayout* InputLayout = ShaderManager->GetInputLayoutByKey(VertexKey);
    
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->IASetPrimitiveTopology(Topology);
}

void FEditorRenderPass::BindBuffers(const FDebugPrimitiveData& InPrimitiveData) const
{
    constexpr UINT Offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &InPrimitiveData.VertexInfo.VertexBuffer, &InPrimitiveData.VertexInfo.Stride, &Offset);
    Graphics->DeviceContext->IASetIndexBuffer(InPrimitiveData.IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
}

void FEditorRenderPass::PrepareRenderArr()
{
    const EWorldType WorldType = GEngine->ActiveWorld->WorldType;
    if (WorldType != EWorldType::Editor && WorldType != EWorldType::PhysicsAssetViewer)
    {
        return;
    }
    
    for (const auto* Actor : TObjectRange<AActor>())
    {
        for (const auto* Component : Actor->GetComponents())
        {
            if (WorldType == EWorldType::PhysicsAssetViewer)
            {
                if (USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(Component))
                {
                    if (USkeletalMesh* SkelMesh = SkelMeshComp->GetSkeletalMeshAsset())
                    {
                        if (SkelMesh->GetPhysicsAsset())
                        {
                            PreviewMesh.Add(SkelMeshComp);
                        }
                    }
                }
            }
            else
            {
                // AABB용 static mesh component
                if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Component))
                {
                    if (!StaticMesh->IsA<UGizmoBaseComponent>())
                    {
                        Resources.Components.StaticMeshComponent.Add(StaticMesh);
                    }
                }

                // light
                if (ULightComponentBase* Light = Cast<ULightComponentBase>(Component))
                {
                    Resources.Components.Light.Add(Light);
                }

                // fog
                if (UHeightFogComponent* Fog = Cast<UHeightFogComponent>(Component))
                {
                    Resources.Components.Fog.Add(Fog);
                }

                if (USphereComponent* SphereComponent = Cast<USphereComponent>(Component))
                {
                    Resources.Components.SphereComponents.Add(SphereComponent);
                }

                if (UBoxComponent* BoxComponent = Cast<UBoxComponent>(Component))
                {
                    Resources.Components.BoxComponents.Add(BoxComponent);
                }
            
                if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(Component))
                {
                    Resources.Components.CapsuleComponents.Add(CapsuleComponent);
                }
            }
        }
    }
}

void FEditorRenderPass::ClearRenderArr()
{
    Resources.Components.StaticMeshComponent.Empty();
    Resources.Components.Light.Empty();
    Resources.Components.Fog.Empty();
    Resources.Components.SphereComponents.Empty();
    Resources.Components.CapsuleComponents.Empty();
    Resources.Components.BoxComponents.Empty();

    PreviewMesh.Empty();
}

void FEditorRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FEditorRenderPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

// 꼼수로 이미 로드된 리소스를 사용
// GUObjectArray에 안올라가게 우회
void FEditorRenderPass::LazyLoad()
{
    // Resourcemanager에서 로드된 texture의 포인터를 가져옴
    // FResourceManager::Initialize에 이미 추가되어 있어야 함
    Resources.IconTextures[EIconType::DirectionalLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/DirectionalLight_64x.png");
    Resources.IconTextures[EIconType::PointLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/PointLight_64x.png");
    Resources.IconTextures[EIconType::SpotLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/SpotLight_64x.png");
    Resources.IconTextures[EIconType::AmbientLight] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/AmbientLight_64x.png");
    Resources.IconTextures[EIconType::ExponentialFog] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/ExponentialHeightFog_64.png");
    Resources.IconTextures[EIconType::AtmosphericFog] = FEngineLoop::ResourceManager.GetTexture(L"Assets/Editor/Icon/AtmosphericFog_64.png");

    // Gizmo arrow 로드
    FStaticMeshRenderData* RenderData = FObjManager::GetStaticMesh(L"Assets/Editor/Gizmo/GizmoTranslationZ.obj")->GetRenderData();

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    
    Resources.Primitives.Arrow.VertexInfo.VertexBuffer = VertexInfo.VertexBuffer;
    Resources.Primitives.Arrow.VertexInfo.NumVertices = VertexInfo.NumVertices;
    Resources.Primitives.Arrow.VertexInfo.Stride = sizeof(FStaticMeshVertex); // Directional Light의 Arrow에 해당됨
    Resources.Primitives.Arrow.IndexInfo.IndexBuffer = IndexInfo.IndexBuffer;
    Resources.Primitives.Arrow.IndexInfo.NumIndices = IndexInfo.NumIndices;
}

void FEditorRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    static bool IsLoaded = false;
    if (!IsLoaded)
    {
        LazyLoad();
        IsLoaded = true;
    }

    const uint64 ShowFlag = Viewport->GetShowFlag();

    BindRenderTarget(Viewport);    

    if (ShowFlag & EEngineShowFlags::SF_LightWireframe)
    {
        RenderPointlightInstanced(ShowFlag);
        RenderSpotlightInstanced(ShowFlag);
    }

    if (ShowFlag & EEngineShowFlags::SF_Collision)
    {
        RenderBoxInstanced(ShowFlag);
        RenderSphereInstanced(ShowFlag);
        RenderCapsuleInstanced(ShowFlag);
    }

    RenderArrowInstanced();
    //RenderIcons(World, ActiveViewport); // 기존 렌더패스에서 아이콘 렌더하고 있으므로 제거

    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    ID3D11Buffer* NullBuffer[1] = { nullptr };
    Graphics->DeviceContext->VSSetConstantBuffers(11, 1, NullBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(11, 1, NullBuffer);
}

void FEditorRenderPass::RenderPointlightInstanced(uint64 ShowFlag)
{
    BindShaderResource(L"SphereVS", L"SpherePS", D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    BindBuffers(Resources.Primitives.Sphere);

    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugSphere> BufferAll;
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComp))
        {
            if (ShowFlag & EEngineShowFlags::SF_LightWireframeSelectedOnly)
            {
                if (Cast<UEditorEngine>(GEngine)->GetSelectedActor())
                {
                    if (Cast<UEditorEngine>(GEngine)->GetSelectedActor()->GetComponents().Contains(PointLightComp))
                    {
                        FConstantBufferDebugSphere b;
                        b.Position = PointLightComp->GetComponentLocation();
                        b.Radius = PointLightComp->GetRadius();
                        BufferAll.Add(b);
                    }
                }
            }
            else
            {
                FConstantBufferDebugSphere b;
                b.Position = PointLightComp->GetComponentLocation();
                b.Radius = PointLightComp->GetRadius();
                BufferAll.Add(b);
            }
        }
    }
    
    BufferManager->BindConstantBuffer("SphereConstantBuffer", 11, EShaderStage::Vertex);
    int BufferIndex = 0;
    for (int i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeSphere) * ConstantBufferSizeSphere; ++i)
    {
        TArray<FConstantBufferDebugSphere> SubBuffer;
        for (int j = 0; j < ConstantBufferSizeSphere; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            BufferManager->UpdateConstantBuffer<FConstantBufferDebugSphere>(TEXT("SphereConstantBuffer"), SubBuffer);
            Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Sphere.IndexInfo.NumIndices, SubBuffer.Num(), 0, 0, 0);
        }
    }
}

void FEditorRenderPass::RenderSpotlightInstanced(uint64 ShowFlag)
{
    BindShaderResource(L"ConeVS", L"ConePS", D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugCone> BufferAll;
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComp))
        {
            if (ShowFlag & EEngineShowFlags::SF_LightWireframeSelectedOnly)
            {
                if (Cast<UEditorEngine>(GEngine)->GetSelectedActor())
                {
                    if (Cast<UEditorEngine>(GEngine)->GetSelectedActor()->GetComponents().Contains(SpotLightComp))
                    {
                        FConstantBufferDebugCone b;
                        b.ApexPosition = SpotLightComp->GetComponentLocation();
                        b.Radius = SpotLightComp->GetRadius();
                        b.Direction = SpotLightComp->GetDirection();
                        // Inner Cone
                        b.Angle = SpotLightComp->GetInnerRad();
                        BufferAll.Add(b);
                        // Outer Cone
                        b.Angle = SpotLightComp->GetOuterRad();
                        BufferAll.Add(b);
                    }
                }
            }
            else
            {
                FConstantBufferDebugCone b;
                b.ApexPosition = SpotLightComp->GetComponentLocation();
                b.Radius = SpotLightComp->GetRadius();
                b.Direction = SpotLightComp->GetDirection();
                // Inner Cone
                b.Angle = SpotLightComp->GetInnerRad();
                BufferAll.Add(b);
                // Outer Cone
                b.Angle = SpotLightComp->GetOuterRad();
                BufferAll.Add(b);
            }
        }
    }


    BufferManager->BindConstantBuffer("ConeConstantBuffer", 11, EShaderStage::Vertex);
    int BufferIndex = 0;
    for (int i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeCone) * ConstantBufferSizeCone; ++i)
    {
        TArray<FConstantBufferDebugCone> SubBuffer;
        for (int j = 0; j < ConstantBufferSizeCone; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            BufferManager->UpdateConstantBuffer<FConstantBufferDebugCone>(TEXT("ConeConstantBuffer"), SubBuffer);
            // Only Draw Selected SpotLight's Cone = 2 | Cone: (24 * 2) * 2 + Sphere: (10 * 2) * 2 = 136
            Graphics->DeviceContext->DrawInstanced(136, SubBuffer.Num(), 0, 0);
        }
    }
}

// 사용 안함
void FEditorRenderPass::RenderIcons(const UWorld* World, std::shared_ptr<FEditorViewportClient> ActiveViewport)
{
    // ULightComponentBase::CheckRayIntersection에서도 수정 필요
    // const float IconScale = 0.3;
    // BindShaderResource(Resources.Shaders.Icon);
    // UINT offset = 0;
    // // input vertex index 없음
    //
    // BufferManager->BindConstantBuffer("IconConstantBuffer", 11, EShaderStage::Vertex);
    // for (ULightComponentBase* LightComp : Resources.Components.Light)
    // {
    //     FConstantBufferDebugIcon b;
    //     b.Position = LightComp->GetWorldLocation();
    //     b.Scale = IconScale;
    //     BufferManager->UpdateConstantBuffer<FConstantBufferDebugIcon>(TEXT("IconConstantBuffer"), b);
    //
    //     if (UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(LightComp))
    //     {
    //         UpdateTextureIcon(EIconType::PointLight);
    //     }
    //     else if (USpotLightComponent* SpotLightComp = Cast<USpotLightComponent>(LightComp))
    //     {
    //         UpdateTextureIcon(EIconType::SpotLight);
    //     }
    //     else if (UDirectionalLightComponent* DirectionalLightComp = Cast<UDirectionalLightComponent>(LightComp))
    //     {
    //         UpdateTextureIcon(EIconType::DirectionalLight);
    //     }
    //     else if (UAmbientLightComponent* AmbientLightComp = Cast<UAmbientLightComponent>(LightComp))
    //     {
    //         UpdateTextureIcon(EIconType::AmbientLight);
    //     }
    //     else
    //     {
    //         // 잘못된 light 종류
    //         continue;
    //     };
    //     Graphics->DeviceContext->Draw(6, 0); // 내부에서 버텍스 사용중
    // }
    //
    // for (UHeightFogComponent* FogComp : Resources.Components.Fog)
    // {
    //     FConstantBufferDebugIcon b;
    //     b.Position = FogComp->GetWorldLocation();
    //     b.Scale = IconScale;
    //     BufferManager->UpdateConstantBuffer<FConstantBufferDebugIcon>(TEXT("IconConstantBuffer"), b);
    //     UpdateTextureIcon(EIconType::ExponentialFog);
    //
    //     Graphics->DeviceContext->Draw(6, 0); // 내부에서 버텍스 사용중
    // }
}

// 사용 안함
void FEditorRenderPass::UpdateTextureIcon(EIconType Type)
{
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &Resources.IconTextures[Type]->TextureSRV);
}

void FEditorRenderPass::RenderArrowInstanced()
{
    BindShaderResource(L"ArrowVS", L"ArrowPS", D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    BindBuffers(Resources.Primitives.Arrow);

    // 1. Collect Instance Data
    TArray<FConstantBufferDebugArrow> BufferAll;
    for (ULightComponentBase* LightComp : Resources.Components.Light)
    {
        constexpr float ArrowScale = 1.0f;
        if (UDirectionalLightComponent* DLightComp = Cast<UDirectionalLightComponent>(LightComp))
        {
            FConstantBufferDebugArrow Buf;
            Buf.Position = DLightComp->GetComponentLocation();
            Buf.ScaleXYZ = ArrowScale;
            Buf.Direction = DLightComp->GetDirection();
            Buf.ScaleZ = ArrowScale;
            BufferAll.Add(Buf);
        }
        else if (USpotLightComponent* SpotComp = Cast<USpotLightComponent>(LightComp))
        {
            FConstantBufferDebugArrow Buf;
            Buf.Position = SpotComp->GetComponentLocation();
            Buf.ScaleXYZ = ArrowScale;
            Buf.Direction = SpotComp->GetDirection();
            Buf.ScaleZ = ArrowScale;
            BufferAll.Add(Buf);
        }
    }

    BufferManager->BindConstantBuffer("ArrowConstantBuffer", 11, EShaderStage::Vertex);

    int32 BufferIndex = 0;
    for (int i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeArrow) * ConstantBufferSizeArrow; ++i)
    {
        TArray<FConstantBufferDebugArrow> SubBuffer;
        for (int32 j = 0; j < ConstantBufferSizeArrow; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            BufferManager->UpdateConstantBuffer<FConstantBufferDebugArrow>(TEXT("ArrowConstantBuffer"), SubBuffer);
            Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Arrow.IndexInfo.NumIndices, SubBuffer.Num(), 0, 0, 0);
        }
    }
}

void FEditorRenderPass::RenderBoxInstanced(uint64 ShowFlag)
{
    BindShaderResource(L"BoxVS", L"BoxPS", D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    BindBuffers(Resources.Primitives.Box);
    
    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugBox> BufferAll;
    for (UBoxComponent* BoxComponent : Resources.Components.BoxComponents)
    {
        if (ShowFlag & EEngineShowFlags::SF_CollisionSelectedOnly)
        {
            AActor* Actor = Cast<UEditorEngine>(GEngine)->GetSelectedActor();
            if (Actor && Actor->GetComponents().Contains(BoxComponent))
            {
                FConstantBufferDebugBox b;
                b.WorldMatrix = BoxComponent->GetWorldMatrix();
                b.Extent = BoxComponent->GetBoxExtent();
                BufferAll.Add(b);
            }
        }
        else
        {
            FConstantBufferDebugBox b;
            b.WorldMatrix = BoxComponent->GetWorldMatrix();
            b.Extent = BoxComponent->GetBoxExtent();
            BufferAll.Add(b);
        }
    }

    for (const UStaticMeshComponent* StaticComp : Resources.Components.StaticMeshComponent)
    {
        if (ShowFlag & EEngineShowFlags::SF_CollisionSelectedOnly)
        {
            for (const auto& GeomAttribute : StaticComp->GeomAttributes)
            {
                if (GeomAttribute.GeomType == EGeomType::EBox)
                {
                    FConstantBufferDebugBox b;
                    FMatrix WorldMatrix =
                        FTransform(GeomAttribute.Rotation, GeomAttribute.Offset, GeomAttribute.Extent).ToMatrixWithScale()
                        * StaticComp->GetWorldMatrix().GetMatrixWithoutScale();
                    b.WorldMatrix = WorldMatrix;
                    b.Extent = GeomAttribute.Extent;
                    BufferAll.Add(b);
                }
            }
        }
    }

    for (const USkeletalMeshComponent* SkelMeshComp : PreviewMesh)
    {
        USkeletalMesh* SkeletalMesh = SkelMeshComp->GetSkeletalMeshAsset();
        UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();

        TArray<FMatrix> BoneMatrices;
        SkelMeshComp->GetCurrentGlobalBoneMatrices(BoneMatrices);
        for (const UBodySetup* BodySetup : PhysicsAsset->BodySetups)
        {
            const FName BoneName = BodySetup->BoneName;
            const int32 BoneIndex = SkeletalMesh->GetRefSkeleton().FindRawBoneIndex(BoneName);
            const FMatrix BoneMatrix = BoneMatrices[BoneIndex];
            for (const AggregateGeomAttributes& Attributes : BodySetup->GeomAttributes)
            {
                if (Attributes.GeomType == EGeomType::EBox)
                {
                    FConstantBufferDebugBox b;
                    FMatrix WorldMatrix =
                        FTransform(Attributes.Rotation, Attributes.Offset, Attributes.Extent).ToMatrixWithScale()
                        * BoneMatrix
                        * SkelMeshComp->GetWorldMatrix().GetMatrixWithoutScale();
                    b.WorldMatrix = WorldMatrix;
                    b.Extent = Attributes.Extent;
                    BufferAll.Add(b);
                }
            }
        }
    }
    
    BufferManager->BindConstantBuffer("BoxConstantBuffer", 11, EShaderStage::Vertex);
    int BufferIndex = 0;
    for (uint32 i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeBox) * ConstantBufferSizeBox; ++i)
    {
        TArray<FConstantBufferDebugBox> SubBuffer;
        for (uint32 j = 0; j < ConstantBufferSizeBox; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            BufferManager->UpdateConstantBuffer<FConstantBufferDebugBox>(TEXT("BoxConstantBuffer"), SubBuffer);
            Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Box.IndexInfo.NumIndices, SubBuffer.Num(), 0, 0, 0);
        }
    }
}

void FEditorRenderPass::RenderSphereInstanced(uint64 ShowFlag)
{
    BindShaderResource(L"SphereVS", L"SpherePS", D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    BindBuffers(Resources.Primitives.Sphere);

    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugSphere> BufferAll;
    for (USphereComponent* SphereComponent : Resources.Components.SphereComponents)
    {
        if (ShowFlag & EEngineShowFlags::SF_CollisionSelectedOnly)
        {
            AActor* Actor = Cast<UEditorEngine>(GEngine)->GetSelectedActor();
            if (Actor && Actor->GetComponents().Contains(SphereComponent))
            {
                FConstantBufferDebugSphere b;
                b.Position = SphereComponent->GetComponentLocation();
                b.Radius = SphereComponent->GetRadius();
                BufferAll.Add(b);
            }
        }
        else
        {
            FConstantBufferDebugSphere b;
            b.Position = SphereComponent->GetComponentLocation();
            b.Radius = SphereComponent->GetRadius();
            BufferAll.Add(b);
        }
    }

    for (const UStaticMeshComponent* StaticComp : Resources.Components.StaticMeshComponent)
    {
        if (ShowFlag & EEngineShowFlags::SF_CollisionSelectedOnly)
        {
            for (const auto& GeomAttribute : StaticComp->GeomAttributes)
            {
                if (GeomAttribute.GeomType == EGeomType::ESphere)
                {
                    FConstantBufferDebugSphere b;
                    FMatrix WorldMatrix =
                        FTransform(GeomAttribute.Rotation, GeomAttribute.Offset, GeomAttribute.Extent).ToMatrixWithScale()
                        * StaticComp->GetWorldMatrix().GetMatrixWithoutScale();
                    b.Position = WorldMatrix.GetTranslationVector();
                    b.Radius = GeomAttribute.Extent.X;
                    BufferAll.Add(b);
                }
            }
        }
    }

    for (const USkeletalMeshComponent* SkelMeshComp : PreviewMesh)
    {
        USkeletalMesh* SkeletalMesh = SkelMeshComp->GetSkeletalMeshAsset();
        UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();

        TArray<FMatrix> BoneMatrices;
        SkelMeshComp->GetCurrentGlobalBoneMatrices(BoneMatrices);
        for (const UBodySetup* BodySetup : PhysicsAsset->BodySetups)
        {
            const FName BoneName = BodySetup->BoneName;
            const int32 BoneIndex = SkeletalMesh->GetRefSkeleton().FindRawBoneIndex(BoneName);
            const FMatrix BoneMatrix = BoneMatrices[BoneIndex];
            for (const AggregateGeomAttributes& Attributes : BodySetup->GeomAttributes)
            {
                if (Attributes.GeomType == EGeomType::ESphere)
                {
                    FConstantBufferDebugSphere b;
                    FMatrix WorldMatrix =
                        FTransform(Attributes.Rotation, Attributes.Offset, Attributes.Extent).ToMatrixWithScale()
                        * BoneMatrix
                        * SkelMeshComp->GetWorldMatrix().GetMatrixWithoutScale();
                    b.Position = WorldMatrix.GetTranslationVector();
                    b.Radius = Attributes.Extent.X;
                    BufferAll.Add(b);
                }
            }
        }
    }

    BufferManager->BindConstantBuffer("SphereConstantBuffer", 11, EShaderStage::Vertex);
    int BufferIndex = 0;
    for (uint32 i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeSphere) * ConstantBufferSizeSphere; ++i)
    {
        TArray<FConstantBufferDebugSphere> SubBuffer;
        for (uint32 j = 0; j < ConstantBufferSizeSphere; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }

        if (SubBuffer.Num() > 0)
        {
            BufferManager->UpdateConstantBuffer<FConstantBufferDebugSphere>(TEXT("SphereConstantBuffer"), SubBuffer);
            Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Sphere.IndexInfo.NumIndices, SubBuffer.Num(), 0, 0, 0);
        }
    }
}

void FEditorRenderPass::RenderCapsuleInstanced(uint64 ShowFlag)
{
    BindShaderResource(L"CapsuleVS", L"CapsulePS", D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    
    // 위치랑 bounding box 크기 정보 가져오기
    TArray<FConstantBufferDebugCapsule> BufferAll;
    for (UShapeComponent* ShapeComponent : Resources.Components.CapsuleComponents)
    {
        if (UCapsuleComponent* CapsuleComponent = Cast<UCapsuleComponent>(ShapeComponent))
        {
            if (ShowFlag & EEngineShowFlags::SF_CollisionSelectedOnly)
            {
                AActor* Actor = Cast<UEditorEngine>(GEngine)->GetSelectedActor();
                if (Actor && Actor->GetComponents().Contains(CapsuleComponent))
                {
                    FConstantBufferDebugCapsule b;
                    b.WorldMatrix = CapsuleComponent->GetWorldMatrix();
                    b.Height = CapsuleComponent->GetHalfHeight();
                    b.Radius = CapsuleComponent->GetRadius();
                    BufferAll.Add(b);
                }
            }
            else
            {
                FConstantBufferDebugCapsule b;
                b.WorldMatrix = CapsuleComponent->GetWorldMatrix();
                b.Height = CapsuleComponent->GetHalfHeight();
                b.Radius = CapsuleComponent->GetRadius();
                BufferAll.Add(b);
            }
        }
    }

    for (const UStaticMeshComponent* StaticComp : Resources.Components.StaticMeshComponent)
    {
        if (ShowFlag & EEngineShowFlags::SF_CollisionSelectedOnly)
        {
            for (const auto& GeomAttribute : StaticComp->GeomAttributes)
            {
                if (GeomAttribute.GeomType == EGeomType::ECapsule)
                {
                    FConstantBufferDebugCapsule b;
                    FMatrix WorldMatrix =
                        FTransform(GeomAttribute.Rotation, GeomAttribute.Offset, GeomAttribute.Extent).ToMatrixWithScale()
                        * StaticComp->GetWorldMatrix().GetMatrixWithoutScale();
                    b.WorldMatrix = WorldMatrix;
                    b.Radius = GeomAttribute.Extent.X;
                    b.Height = GeomAttribute.Extent.Z;
                    BufferAll.Add(b);
                }
            }
        }
    }

    for (const USkeletalMeshComponent* SkelMeshComp : PreviewMesh)
    {
        USkeletalMesh* SkeletalMesh = SkelMeshComp->GetSkeletalMeshAsset();
        UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset();

        TArray<FMatrix> BoneMatrices;
        SkelMeshComp->GetCurrentGlobalBoneMatrices(BoneMatrices);
        for (const UBodySetup* BodySetup : PhysicsAsset->BodySetups)
        {
            const FName BoneName = BodySetup->BoneName;
            const int32 BoneIndex = SkeletalMesh->GetRefSkeleton().FindRawBoneIndex(BoneName);
            const FMatrix BoneMatrix = BoneMatrices[BoneIndex];
            for (const AggregateGeomAttributes& Attributes : BodySetup->GeomAttributes)
            {
                if (Attributes.GeomType == EGeomType::ECapsule)
                {
                    FConstantBufferDebugCapsule b;
                    FMatrix WorldMatrix =
                        FTransform(Attributes.Rotation, Attributes.Offset, Attributes.Extent).ToMatrixWithScale()
                        * BoneMatrix
                        * SkelMeshComp->GetWorldMatrix().GetMatrixWithoutScale();
                    b.WorldMatrix = WorldMatrix;
                    b.Radius = Attributes.Extent.X;
                    b.Height = Attributes.Extent.Z;
                    BufferAll.Add(b);
                }
            }
        }
    }
    
    BufferManager->BindConstantBuffer("CapsuleConstantBuffer", 11, EShaderStage::Vertex);
    int BufferIndex = 0;
    for (int i = 0; i < (1 + BufferAll.Num() / ConstantBufferSizeCapsule) * ConstantBufferSizeCapsule; ++i)
    {
        TArray<FConstantBufferDebugCapsule> SubBuffer;
        for (int j = 0; j < ConstantBufferSizeCapsule; ++j)
        {
            if (BufferIndex < BufferAll.Num())
            {
                SubBuffer.Add(BufferAll[BufferIndex]);
                ++BufferIndex;
            }
            else
            {
                break;
            }
        }
    
        if (SubBuffer.Num() > 0)
        {
            BufferManager->UpdateConstantBuffer<FConstantBufferDebugCapsule>(TEXT("CapsuleConstantBuffer"), SubBuffer);
            //Graphics->DeviceContext->DrawIndexedInstanced(Resources.Primitives.Capsule.IndexInfo.NumIndices, SubBuffer.Num(), 0, 0, 0);

            // 수평 링 : stacks + 1개, 수직 줄 stacks 개
            Graphics->DeviceContext->DrawInstanced(1184, SubBuffer.Num(), 0, 0);
        }
    }
}
