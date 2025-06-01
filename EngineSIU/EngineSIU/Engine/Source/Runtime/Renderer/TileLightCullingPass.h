#pragma once
#include "RenderPassBase.h"
#include "EngineBaseTypes.h"

#include "Define.h"
#include <d3d11.h>

class FDXDShaderManager;
class FGraphicsDevice;
class FDXDBufferManager;

class USpotLightComponent;
class UPointLightComponent;

struct FPointLightGPU
{
    FVector Position;
    float Radius;
    FVector Direction;
    float Padding; // 16바이트 정렬용
};

struct FSpotLightGPU
{
    FVector Position;
    float Radius; // SpotLight가 뻗는 최대 길이 - Attenuation Rad
    FVector Direction;
    float Angle; // Outer Angle(도) : 최대 각도
};

struct TileLightCullSettings
{
    UINT ScreenSize[2];
    UINT TileSize[2];

    float NearZ;
    float FarZ;

    FMatrix ViewMatrix;
    FMatrix ProjectionMatrix;
    FMatrix InvProjectionMatrix;

    UINT NumPointLights;
    UINT NumSpotLights;
    UINT Enable25DCulling;
};

class FTileLightCullingPass : public FRenderPassBase
{
public:
    FTileLightCullingPass() = default;
    virtual ~FTileLightCullingPass() override = default;

    void ResizeTiles(UINT InWidth, UINT InHeight);
    
    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManage) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    void CreatePointLightBufferGPU();
    void CreateSpotLightBufferGPU();
    void CreateViews();
    void CreateBuffers(uint32 InWidth, uint32 InHeight);
    virtual void Release() override;
    void Dispatch(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    void ClearUAVs() const;
    void UpdateTileLightConstantBuffer(const std::shared_ptr<FEditorViewportClient>& Viewport) const;

    void ResizeViewBuffers(uint32 InWidth, uint32 InHeight);

    // UAV 결과를 파싱하여 타일별 영향을 주는 전역 조명 인덱스로 바꾸는 함수
    bool CopyLightIndexMaskBufferToCPU(TArray<uint32>& OutData, ID3D11Buffer*& LightIndexMaskBuffer) const;
    void ParseCulledLightMaskData();

    TArray<UPointLightComponent*> GetPointLights() { return PointLights; }
    TArray<USpotLightComponent*>  GetSpotLights()  { return SpotLights; }

    void SetDepthSRV(ID3D11ShaderResourceView* InDepthSRV) { DepthSRV = InDepthSRV; }

    ID3D11ShaderResourceView* GetPerTilePointLightIndexMaskBufferSRV() const { return PerTilePointLightIndexMaskBufferSRV; }
    ID3D11ShaderResourceView* GetPerTileSpotLightIndexMaskBufferSRV()  const { return PerTileSpotLightIndexMaskBufferSRV; }

    TArray<uint32> GetCulledPointLightMaskData() const { return CulledPointLightMaskData; }
    TArray<uint32> GetCulledSpotLightMaskData()  const { return CulledSpotLightMaskData; }

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    
    virtual void CreateResource() override;
    
private:
    ID3D11ComputeShader* ComputeShader;

    TArray<UPointLightComponent*> PointLights;
    TArray<USpotLightComponent*>  SpotLights;

    ID3D11Buffer*               PointLightBuffer = nullptr;       // PointLight GPU 버퍼    
    ID3D11ShaderResourceView*   PointLightBufferSRV = nullptr;    // PointLight 버퍼 SRV (StructruredBuffer)

    ID3D11Buffer*               SpotLightBuffer = nullptr;        // SpotLight GPU 버퍼    
    ID3D11ShaderResourceView*   SpotLightBufferSRV = nullptr;     // SpotLight 버퍼 SRV (StructruredBuffer)

    ID3D11ShaderResourceView*   DepthSRV = nullptr;               // 깊이 버퍼 SRV

    ID3D11Buffer*               PerTilePointLightIndexMaskBuffer = nullptr;       // 각 타일별 PointLight 마스크 ID
    ID3D11UnorderedAccessView*  PerTilePointLightIndexMaskBufferUAV = nullptr;    // 각 타일별 PointLight 마스크 ID UAV
    ID3D11ShaderResourceView*   PerTilePointLightIndexMaskBufferSRV = nullptr;    // 각 타일별 PointLight 마스크 ID SRV

    ID3D11Buffer*               PerTileSpotLightIndexMaskBuffer = nullptr;        // 각 타일별 SpotLight 마스크 ID
    ID3D11UnorderedAccessView*  PerTileSpotLightIndexMaskBufferUAV = nullptr;     // 각 타일별 SpotLight 마스크 ID UAV
    ID3D11ShaderResourceView*   PerTileSpotLightIndexMaskBufferSRV = nullptr;     // 각 타일별 SpotLight 마스크 ID SRV

    ID3D11Buffer*               CulledPointLightIndexMaskBuffer = nullptr;        // Culling 된 PointLight 마스크 ID 버퍼
    ID3D11UnorderedAccessView*  CulledPointLightIndexMaskBufferUAV = nullptr;     // Culling 된 PointLight 마스크 ID 버퍼 UAV

    ID3D11Buffer*               CulledSpotLightIndexMaskBuffer = nullptr;         // Culling 된 SpotLight 마스크 ID 버퍼
    ID3D11UnorderedAccessView*  CulledSpotLightIndexMaskBufferUAV = nullptr;      // Culling 된 SpotLight 마스크 ID 버퍼 UAV

    TArray<uint32> CulledPointLightMaskData;
    TArray<uint32> CulledSpotLightMaskData;

    ID3D11Texture2D*            DebugHeatmapTexture = nullptr;    // 디버그용 히트맵 텍스처
    ID3D11UnorderedAccessView*  DebugHeatmapUAV = nullptr;        // 디버그용 히트맵 UAV
    ID3D11ShaderResourceView*   DebugHeatmapSRV = nullptr;        // 디버그용 히트맵 SRV

    ID3D11Buffer* TileLightConstantBuffer = nullptr;

    static constexpr uint32 TILE_SIZE = 16;
    static constexpr uint32 MAX_LIGHTS_PER_TILE = 1024;
    
    uint32 TILE_COUNT_X;
    uint32 TILE_COUNT_Y;
    uint32 TILE_COUNT;                          // 타일의 총 개수 (몇개 타일로 나눌지에 따라 결정)
    uint32 SHADER_ENTITY_TILE_BUCKET_COUNT;     // 한 타일이 가질 수 있는 조명 ID를 비트마스크로 표현한 총 슬롯 수

    // 한 타일에 1024개의 라이트 인덱스를 마스크 비트로 저장하려면 : 1024 / sizeof(uint32) = 32
};

