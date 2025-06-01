#include "TileLightCullingPass.h"

#include "RendererHelpers.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"
#include "LevelEditor/SLevelEditor.h"
#include "UObject/Casts.h"
#include "Engine/EditorEngine.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "UObject/UObjectIterator.h"

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

#define PRINTDEBUG FALSE


void FTileLightCullingPass::ResizeTiles(const UINT InWidth, const UINT InHeight)
{
    TILE_COUNT_X = (InWidth + TILE_SIZE - 1) / TILE_SIZE;
    TILE_COUNT_Y = (InHeight + TILE_SIZE - 1) / TILE_SIZE;
    TILE_COUNT = TILE_COUNT_X * TILE_COUNT_Y;
    SHADER_ENTITY_TILE_BUCKET_COUNT = MAX_LIGHTS_PER_TILE / 32;
}

void FTileLightCullingPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManage);
}

void FTileLightCullingPass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<ULightComponentBase>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Iter))
            {
                PointLights.Add(PointLight);
            }
            else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Iter))
            {
                SpotLight->GetDirection();
                SpotLights.Add(SpotLight);
            }
            // [주의] : Directional Light에 대한 CasCade Shadow Map을 만들 때에 아래 View,Proj 갱신을 전제
            Iter->UpdateViewMatrix();
            Iter->UpdateProjectionMatrix();
        }
    }

    CreatePointLightBufferGPU();
    CreateSpotLightBufferGPU();
}

void FTileLightCullingPass::ClearRenderArr()
{
    ClearUAVs();

    PointLights.Empty();
    SpotLights.Empty();
}

void FTileLightCullingPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->PSSetConstantBuffers(8, 1, &TileLightConstantBuffer);
    UpdateTileLightConstantBuffer(Viewport);
    
    DepthSRV = Viewport->GetViewportResource()->GetDepthStencil(EResourceType::ERT_Debug)->SRV;
    ComputeShader = ShaderManager->GetComputeShaderByKey(L"TileLightCullingComputeShader");
    Dispatch(Viewport);

    if (Viewport->GetViewMode() == EViewModeIndex::VMI_LightHeatMap)
    {
        // 디버깅 용도
        Graphics->DeviceContext->PSSetShaderResources(
            static_cast<UINT>(EShaderSRVSlot::SRV_Debug),
            1,
            &DebugHeatmapSRV
        );
    }
}

void FTileLightCullingPass::Dispatch(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    // 한 스레드 그룹(groupSizeX, groupSizeY)은 16x16픽셀 영역처리
    const UINT GroupSizeX = static_cast<UINT>((Viewport->GetD3DViewport().Width + TILE_SIZE - 1) / TILE_SIZE);
    const UINT GroupSizeY = static_cast<UINT>((Viewport->GetD3DViewport().Height + TILE_SIZE - 1) / TILE_SIZE);

    Graphics->DeviceContext->CSSetConstantBuffers(0, 1, &TileLightConstantBuffer);

    // 1. SRV (전역 Light 정보) 바인딩
    if (PointLightBufferSRV)
    {
        Graphics->DeviceContext->CSSetShaderResources(0, 1, &PointLightBufferSRV);                  // register(t0)
    }
    if (SpotLightBufferSRV)
    {
        Graphics->DeviceContext->CSSetShaderResources(2, 1, &SpotLightBufferSRV);                  // register(t0)
    }
    if (DepthSRV)
    {
        Graphics->DeviceContext->CSSetShaderResources(1, 1, &DepthSRV);                  // register(t1)
    }

    // 2. UAV 바인딩
    ID3D11UnorderedAccessView* CSUAVs[] = {
        PerTilePointLightIndexMaskBufferUAV,    // u0
        PerTileSpotLightIndexMaskBufferUAV,     // u1
        CulledPointLightIndexMaskBufferUAV,     // u2
        CulledSpotLightIndexMaskBufferUAV,      // u3
        DebugHeatmapUAV,                        // u4
    };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 5, CSUAVs, nullptr);

    // 3. 셰이더 바인딩
    Graphics->DeviceContext->CSSetShader(ComputeShader, nullptr, 0);

    // 4. 디스패치
    Graphics->DeviceContext->Dispatch(GroupSizeX, GroupSizeY, 1);

    // 5-1. UAV 바인딩 해제 (다른 렌더패스에서 사용하기 위함)
    ID3D11UnorderedAccessView* NullUAV[5] = { nullptr };
    Graphics->DeviceContext->CSSetUnorderedAccessViews(0, 5, NullUAV, nullptr);

    // 5-2. SRV 해제
    ID3D11ShaderResourceView* NullSRVs[2] = { nullptr };
    Graphics->DeviceContext->CSSetShaderResources(0, 2, NullSRVs);

    // 5-3. 상수버퍼 해제
    ID3D11Buffer* NullBuffer[1] = { nullptr };
    Graphics->DeviceContext->CSSetConstantBuffers(0, 1, NullBuffer);
}

void FTileLightCullingPass::CreateResource()
{
    // Compute Shader 생성
    const HRESULT Result = ShaderManager->AddComputeShader(L"TileLightCullingComputeShader", L"Shaders/TileLightCullingComputeShader.hlsl", "mainCS");
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to Compile Compute Shader!"));
    }
    ComputeShader = ShaderManager->GetComputeShaderByKey(L"TileLightCullingComputeShader");

    ResizeTiles(Graphics->ScreenWidth, Graphics->ScreenHeight); // 시작은 전체 크기

    // 한 타일이 가질 수 있는 조명 ID 목록을 비트마스크로 표현한 총 슬롯 수
    CreateViews();
    CreateBuffers(Graphics->ScreenWidth, Graphics->ScreenHeight); // 시작은 전체 크기
}

void FTileLightCullingPass::CreatePointLightBufferGPU()
{
    if (PointLights.Num() == 0)
    {
        return;
    }

    TArray<FPointLightGPU> Lights;

    for (UPointLightComponent* LightComp : PointLights)
    {
        if (!LightComp)
        {
            continue;
        }
        FPointLightGPU LightData;
        LightData = {
            .Position = LightComp->GetComponentLocation(),
            .Radius = LightComp->GetRadius(),
            .Direction = LightComp->GetUpVector(),
            .Padding = 0.0f
        };
        Lights.Add(LightData);
    }

    D3D11_BUFFER_DESC Desc = {};
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.ByteWidth = sizeof(FPointLightGPU) * Lights.Num();
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.StructureByteStride = sizeof(FPointLightGPU);
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Lights.GetData();

    SAFE_RELEASE(PointLightBuffer)
    SAFE_RELEASE(PointLightBufferSRV)

    HRESULT Result = Graphics->Device->CreateBuffer(&Desc, &InitData, &PointLightBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Light Structured Buffer!"));
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = Lights.Num();

    Result = Graphics->Device->CreateShaderResourceView(PointLightBuffer, &SrvDesc, &PointLightBufferSRV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Light Buffer SRV!"));
    }
}

void FTileLightCullingPass::CreateSpotLightBufferGPU()
{
    if (SpotLights.Num() == 0)
    {
        return;
    }

    TArray<FSpotLightGPU> Lights;

    for (USpotLightComponent* LightComp : SpotLights)
    {
        if (!LightComp)
        {
            continue;
        }
        FSpotLightGPU LightData;
        LightData = {
            .Position = LightComp->GetComponentLocation(),
            .Radius = LightComp->GetRadius(),
            .Direction = LightComp->GetDirection(),
            .Angle = LightComp->GetOuterDegree(),
        };
        Lights.Add(LightData);
    }

    D3D11_BUFFER_DESC Desc = {};
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.ByteWidth = sizeof(FSpotLightGPU) * Lights.Num();
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.StructureByteStride = sizeof(FSpotLightGPU);
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Lights.GetData();

    SAFE_RELEASE(SpotLightBuffer)
    SAFE_RELEASE(SpotLightBufferSRV)

    HRESULT Result = Graphics->Device->CreateBuffer(&Desc, &InitData, &SpotLightBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Light Structured Buffer!"));
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = Lights.Num();

    Result = Graphics->Device->CreateShaderResourceView(SpotLightBuffer, &SrvDesc, &SpotLightBufferSRV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Light Buffer SRV!"));
    }
}

void FTileLightCullingPass::CreateViews()
{
    // 2. entityTiles UAV buffer (tile mask 결과용)
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.ByteWidth = sizeof(uint32) * TILE_COUNT * SHADER_ENTITY_TILE_BUCKET_COUNT;
    BufferDesc.Usage = D3D11_USAGE_DEFAULT;
    BufferDesc.StructureByteStride = sizeof(uint32);
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

    D3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    UAVDesc.Buffer.FirstElement = 0;
    UAVDesc.Buffer.NumElements = TILE_COUNT * SHADER_ENTITY_TILE_BUCKET_COUNT;

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.Buffer.FirstElement = 0;
    SRVDesc.Buffer.NumElements = TILE_COUNT * SHADER_ENTITY_TILE_BUCKET_COUNT;

    // Per Tile PointLight Index Buffer
    HRESULT Result = Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &PerTilePointLightIndexMaskBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Tile UAV Buffer!"));
    }

    Result = Graphics->Device->CreateUnorderedAccessView(PerTilePointLightIndexMaskBuffer, &UAVDesc, &PerTilePointLightIndexMaskBufferUAV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Tile UAV!"));
    }

    Result = Graphics->Device->CreateShaderResourceView(PerTilePointLightIndexMaskBuffer, &SRVDesc, &PerTilePointLightIndexMaskBufferSRV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Tile SRV!"));
    }

    // SpotLight Buffer
    Result = Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &PerTileSpotLightIndexMaskBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Tile SpotLight UAV Buffer!"));
    }

    Result = Graphics->Device->CreateUnorderedAccessView(PerTileSpotLightIndexMaskBuffer, &UAVDesc, &PerTileSpotLightIndexMaskBufferUAV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Tile Spot UAV!"));
    }

    Result = Graphics->Device->CreateShaderResourceView(PerTileSpotLightIndexMaskBuffer, &SRVDesc, &PerTileSpotLightIndexMaskBufferSRV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Tile Spot SRV!"));
    }

    BufferDesc.ByteWidth = sizeof(uint32) * SHADER_ENTITY_TILE_BUCKET_COUNT;
    SRVDesc.Buffer.NumElements = SHADER_ENTITY_TILE_BUCKET_COUNT;
    UAVDesc.Buffer.NumElements = SHADER_ENTITY_TILE_BUCKET_COUNT;

    // Culled PointLight Index Buffer
    Result = Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &CulledPointLightIndexMaskBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Culled PointLight Index Buffer!"));
    }

    Result = Graphics->Device->CreateUnorderedAccessView(CulledPointLightIndexMaskBuffer, &UAVDesc, &CulledPointLightIndexMaskBufferUAV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Culled PointLight Index UAV!"));
    }

    // Culled SpotLight Index Buffer
    Result = Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &CulledSpotLightIndexMaskBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Culled SpotLight Index Buffer!"));
    }

    Result = Graphics->Device->CreateUnorderedAccessView(CulledSpotLightIndexMaskBuffer, &UAVDesc, &CulledSpotLightIndexMaskBufferUAV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Culled SpotLight Index UAV!"));
    }
}

void FTileLightCullingPass::CreateBuffers(uint32 InWidth, uint32 InHeight)
{
    // 3. Debug heatmap 텍스처 + UAV (디버깅용)
    D3D11_TEXTURE2D_DESC HeatMapDesc = {};
    HeatMapDesc.Width = InWidth;
    HeatMapDesc.Height = InHeight;
    HeatMapDesc.MipLevels = 1;
    HeatMapDesc.ArraySize = 1;
    HeatMapDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // R32G32B32A32_FLOAT
    HeatMapDesc.SampleDesc.Count = 1;
    HeatMapDesc.Usage = D3D11_USAGE_DEFAULT;
    HeatMapDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;

    HRESULT Result = Graphics->Device->CreateTexture2D(&HeatMapDesc, nullptr, &DebugHeatmapTexture);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Heatmap Texture!"));
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC DebugUAVDesc = {};
    DebugUAVDesc.Format = HeatMapDesc.Format;
    DebugUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

    Result = Graphics->Device->CreateUnorderedAccessView(DebugHeatmapTexture, &DebugUAVDesc, &DebugHeatmapUAV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Heatmap UAV!"));
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = HeatMapDesc.Format;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;

    Result = Graphics->Device->CreateShaderResourceView(DebugHeatmapTexture, &SRVDesc, &DebugHeatmapSRV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Heatmap SRV!"));
    }

    //// 4. TileLight Culling 설정용 ConstantBuffer (2.5D 켜기/끄기 등)
    D3D11_BUFFER_DESC CBDesc = {};
    CBDesc.Usage = D3D11_USAGE_DYNAMIC;
    CBDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    CBDesc.ByteWidth = (sizeof(TileLightCullSettings) + 0xf) & 0xfffffff0; // struct 정의 필요
    CBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    Result = Graphics->Device->CreateBuffer(&CBDesc, nullptr, &TileLightConstantBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create TileLight Constant Buffer!"));
    }
}

void FTileLightCullingPass::Release()
{
    FRenderPassBase::Release();
    
    // Compute Shader Release는 ShaderManager에서 관리
    SAFE_RELEASE(PerTilePointLightIndexMaskBuffer)
    SAFE_RELEASE(PerTilePointLightIndexMaskBufferUAV)
    SAFE_RELEASE(PerTilePointLightIndexMaskBufferSRV)

    SAFE_RELEASE(PerTileSpotLightIndexMaskBuffer)
    SAFE_RELEASE(PerTileSpotLightIndexMaskBufferUAV)
    SAFE_RELEASE(PerTileSpotLightIndexMaskBufferSRV)

    SAFE_RELEASE(CulledPointLightIndexMaskBuffer)
    SAFE_RELEASE(CulledPointLightIndexMaskBufferUAV)

    SAFE_RELEASE(CulledSpotLightIndexMaskBuffer)
    SAFE_RELEASE(CulledSpotLightIndexMaskBufferUAV)
    
    SAFE_RELEASE(DebugHeatmapTexture)
    SAFE_RELEASE(DebugHeatmapUAV)
    SAFE_RELEASE(DebugHeatmapSRV)

    SAFE_RELEASE(TileLightConstantBuffer)
    
    SAFE_RELEASE(PointLightBuffer)
    SAFE_RELEASE(PointLightBufferSRV)

    SAFE_RELEASE(SpotLightBuffer)
    SAFE_RELEASE(SpotLightBufferSRV)
}

void FTileLightCullingPass::ClearUAVs() const
{
    // UAV 초기화용 zero값
    constexpr UINT ClearColor[4] = { 0, 0, 0, 0 };

    // 1. 타일 마스크 초기화
    Graphics->DeviceContext->ClearUnorderedAccessViewUint(PerTilePointLightIndexMaskBufferUAV, ClearColor);
    Graphics->DeviceContext->ClearUnorderedAccessViewUint(PerTileSpotLightIndexMaskBufferUAV, ClearColor);
    Graphics->DeviceContext->ClearUnorderedAccessViewUint(CulledPointLightIndexMaskBufferUAV, ClearColor);
    Graphics->DeviceContext->ClearUnorderedAccessViewUint(CulledSpotLightIndexMaskBufferUAV, ClearColor);

    // 2. 히트맵 초기화
    constexpr float ClearColorF[4] = { 0, 0, 0, 0 };
    Graphics->DeviceContext->ClearUnorderedAccessViewFloat(DebugHeatmapUAV, ClearColorF);
}

void FTileLightCullingPass::UpdateTileLightConstantBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    // 1. Constant Buffer 업데이트
    TileLightCullSettings Settings;
    Settings.ScreenSize[0] = static_cast<uint32>(Viewport->GetD3DViewport().Width);
    Settings.ScreenSize[1] = static_cast<uint32>(Viewport->GetD3DViewport().Height);
    Settings.TileSize[0] = TILE_SIZE;
    Settings.TileSize[1] = TILE_SIZE;
    Settings.NearZ = Viewport->NearClip;
    Settings.FarZ = Viewport->FarClip;
    Settings.ViewMatrix = Viewport->GetViewMatrix();
    Settings.ProjectionMatrix = Viewport->GetProjectionMatrix();
    Settings.InvProjectionMatrix = FMatrix::Inverse(Viewport->GetProjectionMatrix());
    Settings.NumPointLights = PointLights.Num();
    Settings.NumSpotLights = SpotLights.Num();
    Settings.Enable25DCulling = 1;                      // TODO : IMGUI 연결!

    D3D11_MAPPED_SUBRESOURCE MSR;

    HRESULT Result = Graphics->DeviceContext->Map(TileLightConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MSR);
    if (FAILED(Result)) {
        UE_LOG(ELogLevel::Error, TEXT("Failed to map TileLightConstantBuffer"));
        return;
    }
    memcpy(MSR.pData, &Settings, sizeof(TileLightCullSettings));
    
    Graphics->DeviceContext->Unmap(TileLightConstantBuffer, 0);
}

// Compute Shader에 사용되는 모든 SRV와 UAV를 해제
void FTileLightCullingPass::ResizeViewBuffers(const uint32 InWidth, const uint32 InHeight)
{
    ResizeTiles(InWidth, InHeight);

    Release();

    CreateViews();
    CreateBuffers(InWidth, InHeight);
}

// @todo 그냥 LightIndexMaskBuffer를 만들 때 CPU_ACCESS_READ로 만들면 안되나?
// UAV로 쓰여진 Buffer -> StagingBuffer로 복사하는 코드 (LightIndexMaskBuffer는 USAGE_DEFAULT라서 CPU에서 읽을 수 없음)
bool FTileLightCullingPass::CopyLightIndexMaskBufferToCPU(TArray<uint32>& OutData, ID3D11Buffer*& LightIndexMaskBuffer) const
{
    D3D11_BUFFER_DESC BufferDesc = {};
    LightIndexMaskBuffer->GetDesc(&BufferDesc);
    BufferDesc.Usage = D3D11_USAGE_STAGING;
    BufferDesc.BindFlags = 0; 
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ; // CPU 읽기 가능

    ID3D11Buffer* StagingBuffer = nullptr;

    HRESULT Result = Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &StagingBuffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create staging buffer for TileUAVBuffer"));
        return false;
    }

    Graphics->DeviceContext->CopyResource(StagingBuffer, LightIndexMaskBuffer);

    D3D11_MAPPED_SUBRESOURCE MSR = {};

    Result = Graphics->DeviceContext->Map(StagingBuffer, 0, D3D11_MAP_READ, 0, &MSR);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("TileUAVBuffer Staging Buffer Mapping Failed"));
        SAFE_RELEASE(StagingBuffer)
        return false;
    }

    const int Count = static_cast<int>(BufferDesc.ByteWidth) / sizeof(uint32);
    OutData.SetNum(Count);
    memcpy(OutData.GetData(), MSR.pData, BufferDesc.ByteWidth);

    Graphics->DeviceContext->Unmap(StagingBuffer, 0);
    SAFE_RELEASE(StagingBuffer)

    return true;
}

void FTileLightCullingPass::ParseCulledLightMaskData()
{
    //CulledPointLightMaskData.Empty();
    //CulledSpotLightMaskData.Empty();

    if (!CopyLightIndexMaskBufferToCPU(CulledPointLightMaskData, CulledPointLightIndexMaskBuffer))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to copy Culled PointLight Index Mask Buffer to CPU"));
        return;
    }
    if (!CopyLightIndexMaskBufferToCPU(CulledSpotLightMaskData, CulledSpotLightIndexMaskBuffer))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to copy Culled SpotLight Index Mask Buffer to CPU"));
        return;
    }

    TArray<uint32> PointLightIndices;
    TArray<uint32> SpotLightIndices;

    //FString PointOutput = FString::Printf(TEXT("Culled PointLight Mask Data: "));
    //FString SpotOutput = FString::Printf(TEXT("Culled SpotLight Mask Data: "));

    uint32 BucketPerTile = SHADER_ENTITY_TILE_BUCKET_COUNT;
    for (uint32 Bucket = 0; Bucket < BucketPerTile; Bucket++)
    {
        for (uint32 Bit = 0; Bit < 32; Bit++)
        {
            uint32 PointMask = CulledPointLightMaskData[Bucket];
            if (PointMask & (1 << Bit))
            {
                PointLightIndices.Add(Bucket * 32 + Bit);
                //PointOutput += FString::Printf(TEXT("%d "), Bucket * 32 + Bit);
            }

            uint32 SpotMask = CulledSpotLightMaskData[Bucket];
            if (SpotMask & (1 << Bit))
            {
                SpotLightIndices.Add(Bucket * 32 + Bit);
                //SpotOutput += FString::Printf(TEXT("%d "), Bucket * 32 + Bit);
            }
        }
    }

    CulledPointLightMaskData = PointLightIndices;
    CulledSpotLightMaskData = SpotLightIndices;
    //UE_LOG(ELogLevel::Display, TEXT("Culled PointLight Count: %d"), CulledPointLightMaskData.Num());
    //UE_LOG(ELogLevel::Display, TEXT("%s"), *PointOutput);
    //UE_LOG(ELogLevel::Display, TEXT("Culled SpotLight Count: %d"), CulledSpotLightMaskData.Num());
    //UE_LOG(ELogLevel::Display, TEXT("%s"), *SpotOutput);
}

void FTileLightCullingPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FTileLightCullingPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

