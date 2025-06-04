#include "Define.h"
#include "UObject/Casts.h"
#include "UpdateLightBufferPass.h"

#include <algorithm>
#include <utility>
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/AmbientLightComponent.h"
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UObject/UObjectIterator.h"
#include "TileLightCullingPass.h"
#include "LightDefine.h"
void FUpdateLightBufferPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    FRenderPassBase::Initialize(InBufferManager, InGraphics, InShaderManager);

    CreatePointLightBuffer();
    CreatePointLightPerTilesBuffer();
    CreateSpotLightBuffer();
    CreateSpotLightPerTilesBuffer();
}

void FUpdateLightBufferPass::PrepareRenderArr()
{
    for (const auto Iter : TObjectRange<ULightComponentBase>())
    {
        if (Iter->GetWorld() == GEngine->ActiveWorld)
        {
            if (UPointLightComponent* PointLight = Cast<UPointLightComponent>(Iter))
            {
                //PointLights.Add(PointLight); // 당분간 UnUsed : Structured Buffer로 전달
            }
            else if (USpotLightComponent* SpotLight = Cast<USpotLightComponent>(Iter))
            {
                //SpotLights.Add(SpotLight); // UnUsed : Structured Buffer로 전달
            }
            else if (UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(Iter))
            {
                DirectionalLights.Add(DirectionalLight);
            }
            else if (UAmbientLightComponent* AmbientLight = Cast<UAmbientLightComponent>(Iter))
            {
                AmbientLights.Add(AmbientLight);
            }
        }
    }
}

void FUpdateLightBufferPass::ClearRenderArr()
{
    PointLights.Empty();
    SpotLights.Empty();
    DirectionalLights.Empty();
    AmbientLights.Empty();
}

void FUpdateLightBufferPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    UpdateLightBuffer();

    // 전역 조명 리스트
    Graphics->DeviceContext->PSSetShaderResources(10, 1, &PointLightSRV);
    Graphics->DeviceContext->PSSetShaderResources(11, 1, &SpotLightSRV);
    // 타일별 조명 인덱스 리스트
    Graphics->DeviceContext->PSSetShaderResources(12, 1, &PointLightIndexBufferSRV);
    Graphics->DeviceContext->PSSetShaderResources(13, 1, &SpotLightIndexBufferSRV);
}


void FUpdateLightBufferPass::UpdateLightBuffer() const
{
    FLightInfoBuffer LightBufferData = {};

    int DirectionalLightsCount=0;
    int PointLightsCount=0;
    int SpotLightsCount=0;
    int AmbientLightsCount=0;
    
    for (auto Light : SpotLights)
    {
        if (SpotLightsCount < MAX_SPOT_LIGHT)
        {
            LightBufferData.SpotLights[SpotLightsCount] = Light->GetSpotLightInfo();
            LightBufferData.SpotLights[SpotLightsCount].Position = Light->GetComponentLocation();
            LightBufferData.SpotLights[SpotLightsCount].Direction = Light->GetDirection();
            SpotLightsCount++;
        }
    }

    for (auto Light : PointLights)
    {
        if (PointLightsCount < MAX_POINT_LIGHT)
        {
            LightBufferData.PointLights[PointLightsCount] = Light->GetPointLightInfo();
            LightBufferData.PointLights[PointLightsCount].Position = Light->GetComponentLocation();
            PointLightsCount++;
        }
    }

    for (auto Light : DirectionalLights)
    {
        if (DirectionalLightsCount < MAX_DIRECTIONAL_LIGHT)
        {
            LightBufferData.Directional[DirectionalLightsCount] = Light->GetDirectionalLightInfo();
            LightBufferData.Directional[DirectionalLightsCount].Direction = Light->GetDirection();
            LightBufferData.Directional[DirectionalLightsCount].LightViewProj = Light->GetViewProjectionMatrix();
            LightBufferData.Directional[DirectionalLightsCount].LightInvProj = FMatrix::Inverse(Light->GetProjectionMatrix());
            //ShadowData.LightNearZ = Light->GetShadowNearPlane();
            //ShadowData.LightFrustumWidth = Light->GetShadowFrustumWidth();
                        
            //ShadowData.ShadowMapWidth = Light->GetShadowMapWidth();
            //ShadowData.ShadowMapHeight = Light->GetShadowMapHeight();
           
            DirectionalLightsCount++;
        }
    }

    for (auto Light : AmbientLights)
    {
        if (AmbientLightsCount < MAX_DIRECTIONAL_LIGHT)
        {
            LightBufferData.Ambient[AmbientLightsCount] = Light->GetAmbientLightInfo();
            LightBufferData.Ambient[AmbientLightsCount].AmbientColor = Light->GetLightColor();
            AmbientLightsCount++;
        }
    }
    
    LightBufferData.DirectionalLightsCount = DirectionalLightsCount;
    LightBufferData.PointLightsCount = PointLightsCount;
    LightBufferData.SpotLightsCount = SpotLightsCount;
    LightBufferData.AmbientLightsCount = AmbientLightsCount;

    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);
    
}

void FUpdateLightBufferPass::SetPointLightData(
    const TArray<UPointLightComponent*>& InPointLights, TArray<TArray<uint32>> InPointLightPerTiles)
{
    PointLights = InPointLights; 
    PointLightPerTiles = InPointLightPerTiles;

    uint32 TotalTiles = PointLightPerTiles.Num();
    GPointLightPerTiles.Empty();
    GPointLightPerTiles.SetNum(TotalTiles);

    for (uint32 TileIndex = 0; TileIndex < TotalTiles; ++TileIndex)
    {
        const TArray<uint32>& TileLightList = InPointLightPerTiles[TileIndex];
        PointLightPerTile TileData = {};
        TileData.NumLights = TileLightList.Num();
        TileData.NumLights = FMath::Min<uint32>(TileData.NumLights, MAX_POINTLIGHT_PER_TILE);

        // 각 조명 인덱스를 TileData.Indice 배열에 복사합니다.
        for (uint32 Idx = 0; Idx < TileData.NumLights; ++Idx)
        {
            TileData.Indices[Idx] = TileLightList[Idx];
        }
        GPointLightPerTiles[TileIndex] = TileData;
    }

    UpdatePointLightBuffer();
    UpdatePointLightPerTilesBuffer();
}

void FUpdateLightBufferPass::SetSpotLightData(const TArray<USpotLightComponent*>& InSpotLights, TArray<TArray<uint32>> InSpotLightPerTiles)
{
    SpotLights = InSpotLights;
    SpotLightPerTiles = InSpotLightPerTiles;

    uint32 TotalTiles = SpotLightPerTiles.Num();
    GSpotLightPerTiles.Empty();
    GSpotLightPerTiles.SetNum(TotalTiles);

    for (uint32 TileIndex = 0; TileIndex < TotalTiles; ++TileIndex)
    {
        const TArray<uint32>& TileLightList = InSpotLightPerTiles[TileIndex];
        SpotLightPerTile TileData = {};
        TileData.NumLights = TileLightList.Num();
        TileData.NumLights = FMath::Min<uint32>(TileData.NumLights, MAX_SPOTLIGHT_PER_TILE);

        // 각 조명 인덱스를 TileData.Indice 배열에 복사합니다.
        for (uint32 i = 0; i < TileData.NumLights; ++i)
        {
            TileData.Indices[i] = TileLightList[i];
        }
        GSpotLightPerTiles[TileIndex] = TileData;
    }

    UpdateSpotLightBuffer();
    UpdateSpotLightPerTilesBuffer();
}

void FUpdateLightBufferPass::SetLightData(const TArray<UPointLightComponent*>& InPointLights, const TArray<USpotLightComponent*>& InSpotLights,
    ID3D11ShaderResourceView* InPointLightIndexBufferSRV, ID3D11ShaderResourceView* InSpotLightIndexBufferSRV)
{
    PointLights = InPointLights;
    SpotLights = InSpotLights;
    PointLightIndexBufferSRV = InPointLightIndexBufferSRV;
    SpotLightIndexBufferSRV = InSpotLightIndexBufferSRV;

    UpdatePointLightBuffer();
    UpdateSpotLightBuffer();
}

void FUpdateLightBufferPass::CreatePointLightBuffer()
{
    D3D11_BUFFER_DESC Desc = {};
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.ByteWidth = sizeof(FPointLightInfo) * MAX_NUM_POINTLIGHTS; // TOFIX : 하드코딩 : 10000개 light 받을 수 있음
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Desc.StructureByteStride = sizeof(FPointLightInfo);

    HRESULT hr = Graphics->Device->CreateBuffer(&Desc, nullptr, &PointLightBuffer);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PointLightBuffer"));
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = MAX_NUM_POINTLIGHTS;

    hr = Graphics->Device->CreateShaderResourceView(PointLightBuffer, &SrvDesc, &PointLightSRV);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PointLight SRV"));
    }
}

void FUpdateLightBufferPass::CreateSpotLightBuffer()
{
    D3D11_BUFFER_DESC Desc = {};
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.ByteWidth = sizeof(FSpotLightInfo) * MAX_NUM_SPOTLIGHTS; // TOFIX : 하드코딩 : 10000개 light 받을 수 있음
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Desc.StructureByteStride = sizeof(FSpotLightInfo);
    HRESULT hr = Graphics->Device->CreateBuffer(&Desc, nullptr, &SpotLightBuffer);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create SpotLightBuffer"));
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = MAX_NUM_SPOTLIGHTS;
    hr = Graphics->Device->CreateShaderResourceView(SpotLightBuffer, &SrvDesc, &SpotLightSRV);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create SpotLight SRV"));
    }
}

void FUpdateLightBufferPass::CreatePointLightPerTilesBuffer()
{    
    D3D11_BUFFER_DESC Desc = {};
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.ByteWidth = sizeof(PointLightPerTile) * MAX_TILE;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Desc.StructureByteStride = sizeof(PointLightPerTile);       // 타일당 라이트 : 기존 1024에서 256으로 변경

    /* D3D11_SUBRESOURCE_DATA initData = {};
     initData.pSysMem = GPointLightPerTiles.GetData();*/

    HRESULT hr = Graphics->Device->CreateBuffer(&Desc, nullptr, &PointLightPerTilesBuffer);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PointLightPerTilesBuffer"));
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = MAX_TILE;

    hr = Graphics->Device->CreateShaderResourceView(PointLightPerTilesBuffer, &SrvDesc, &PointLightPerTilesSRV);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create PointLightPerTiles SRV"));
    }
}

void FUpdateLightBufferPass::CreateSpotLightPerTilesBuffer()
{
    D3D11_BUFFER_DESC Desc = {};
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.ByteWidth = sizeof(SpotLightPerTile) * MAX_TILE;
    Desc.Usage = D3D11_USAGE_DEFAULT;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Desc.StructureByteStride = sizeof(SpotLightPerTile);       // 타일당 라이트 : 기존 1024에서 256으로 변경

    HRESULT hr = Graphics->Device->CreateBuffer(&Desc, nullptr, &SpotLightPerTilesBuffer);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create SpotLightPerTilesBuffer"));
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = MAX_TILE;
    hr = Graphics->Device->CreateShaderResourceView(SpotLightPerTilesBuffer, &SrvDesc, &SpotLightPerTilesSRV);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create SpotLightPerTiles SRV"));
    }
}

void FUpdateLightBufferPass::UpdatePointLightBuffer()
{
    if (PointLights.Num() == 0 || !PointLightBuffer)
    {
        return;
    }

    TArray<FPointLightInfo> TempBuffer;
    TempBuffer.SetNum(MAX_NUM_POINTLIGHTS);
    for (uint32 LightIdx = 0; std::cmp_less(LightIdx, PointLights.Num()); ++LightIdx)
    {
        FPointLightInfo& LightInfo = PointLights[LightIdx]->GetPointLightInfo();
        LightInfo.Position = PointLights[LightIdx]->GetComponentLocation();
        for (int ProjectionIndex = 0; ProjectionIndex < 6; ++ProjectionIndex)
        {
            LightInfo.LightViewProjs[ProjectionIndex] = PointLights[LightIdx]->GetViewProjectionMatrix(ProjectionIndex);
        }
        LightInfo.ShadowMapArrayIndex = LightIdx;
        LightInfo.ShadowBias = 0.5f;
        TempBuffer[LightIdx] = LightInfo;
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(PointLightBuffer, 0, nullptr,
        TempBuffer.GetData(), 0, 0);
}
 
void FUpdateLightBufferPass::UpdateSpotLightBuffer()
{
    if (SpotLights.Num() == 0 || !SpotLightBuffer)
    {
        return;
    }
    TArray<FSpotLightInfo> TempBuffer;
    TempBuffer.SetNum(MAX_NUM_SPOTLIGHTS);
    for (uint32 Idx = 0; std::cmp_less(Idx, SpotLights.Num()); ++Idx)
    {
        FSpotLightInfo& LightInfo = SpotLights[Idx]->GetSpotLightInfo();
        LightInfo.Position = SpotLights[Idx]->GetComponentLocation();
        LightInfo.Direction = SpotLights[Idx]->GetDirection();
        LightInfo.LightViewProj = SpotLights[Idx]->GetViewMatrix() * SpotLights[Idx]->GetProjectionMatrix();
        LightInfo.ShadowMapArrayIndex = Idx;
        LightInfo.ShadowBias = 0.5f;
        TempBuffer[Idx] = LightInfo;
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(SpotLightBuffer, 0, nullptr,
        TempBuffer.GetData(), 0, 0);
}

void FUpdateLightBufferPass::UpdatePointLightPerTilesBuffer()
{
    if (GPointLightPerTiles.Num() == 0 || !PointLightPerTilesBuffer)
    {
        return;
    }

    TArray<PointLightPerTile> TempBuffer;
    TempBuffer.SetNum(MAX_TILE);
    for (uint32 Idx = 0; std::cmp_less(Idx, GPointLightPerTiles.Num()); ++Idx)
    {
        TempBuffer[Idx] = GPointLightPerTiles[Idx];
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(PointLightPerTilesBuffer, 0, nullptr,
        TempBuffer.GetData(), 0, 0);
}

void FUpdateLightBufferPass::UpdateSpotLightPerTilesBuffer()
{
    if (GSpotLightPerTiles.Num() == 0 || !SpotLightPerTilesBuffer)
    {
        return;
    }
    TArray<SpotLightPerTile> TempBuffer;
    TempBuffer.SetNum(MAX_TILE);
    for (uint32 Idx = 0; std::cmp_less(Idx, GSpotLightPerTiles.Num()); ++Idx)
    {
        TempBuffer[Idx] = GSpotLightPerTiles[Idx];
    }
    // 이제 TempBuffer에 대해 업데이트
    Graphics->DeviceContext->UpdateSubresource(SpotLightPerTilesBuffer, 0, nullptr,
        TempBuffer.GetData(), 0, 0);
}

void FUpdateLightBufferPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}

void FUpdateLightBufferPass::CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
}
