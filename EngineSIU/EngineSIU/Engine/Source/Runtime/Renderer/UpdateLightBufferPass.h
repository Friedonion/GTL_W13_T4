#pragma once

#include "RenderPassBase.h"
#include "EngineBaseTypes.h"
#include "Container/Set.h"
#include "Define.h"

#define MAX_POINTLIGHT_PER_TILE 256
#define MAX_SPOTLIGHT_PER_TILE 256

class FDXDShaderManager;
class UWorld;
class FEditorViewportClient;

class UPointLightComponent;
class USpotLightComponent;
class UDirectionalLightComponent;
class UAmbientLightComponent;

struct PointLightPerTile {
    uint32 NumLights;
    uint32 Indices[MAX_POINTLIGHT_PER_TILE];
    uint32 Padding[3];
};

struct SpotLightPerTile {
    uint32 NumLights;
    uint32 Indices[MAX_SPOTLIGHT_PER_TILE];
    uint32 Padding[3];
};

class FUpdateLightBufferPass : public FRenderPassBase
{
public:
    FUpdateLightBufferPass() = default;
    virtual ~FUpdateLightBufferPass() override = default;

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;
    virtual void PrepareRenderArr() override;
    virtual void ClearRenderArr() override;
    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    void UpdateLightBuffer() const;

    void SetPointLightData(const TArray<UPointLightComponent*>& InPointLights, TArray<TArray<uint32>> InPointLightPerTiles);
    void SetSpotLightData(const TArray<USpotLightComponent*>& InSpotLights, TArray<TArray<uint32>> InSpotLightPerTiles);
    void SetLightData(const TArray<UPointLightComponent*>& InPointLights, const TArray<USpotLightComponent*>& InSpotLights, ID3D11ShaderResourceView* InPointLightIndexBufferSRV, ID3D11ShaderResourceView* InSpotLightIndexBufferSRV);

    void CreatePointLightBuffer();
    void CreateSpotLightBuffer();

    void CreatePointLightPerTilesBuffer();
    void CreateSpotLightPerTilesBuffer();

    void UpdatePointLightBuffer();
    void UpdateSpotLightBuffer();

    void UpdatePointLightPerTilesBuffer();
    void UpdateSpotLightPerTilesBuffer();

protected:
    virtual void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;
    virtual void CleanUpRender(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

private:
    TArray<USpotLightComponent*> SpotLights;
    TArray<UPointLightComponent*> PointLights;
    TArray<UDirectionalLightComponent*> DirectionalLights;
    TArray<UAmbientLightComponent*> AmbientLights;

    TArray<TArray<uint32>> PointLightPerTiles;
    TArray<PointLightPerTile> GPointLightPerTiles;

    TArray<TArray<uint32>> SpotLightPerTiles;
    TArray<SpotLightPerTile> GSpotLightPerTiles;

    ID3D11Buffer* PointLightBuffer = nullptr;
    ID3D11ShaderResourceView* PointLightSRV = nullptr;

    ID3D11Buffer* SpotLightBuffer = nullptr;
    ID3D11ShaderResourceView* SpotLightSRV = nullptr;
    
    ID3D11Buffer* PointLightPerTilesBuffer = nullptr;
    ID3D11ShaderResourceView* PointLightPerTilesSRV = nullptr;

    ID3D11Buffer* SpotLightPerTilesBuffer = nullptr;
    ID3D11ShaderResourceView* SpotLightPerTilesSRV = nullptr;

    ID3D11ShaderResourceView* PointLightIndexBufferSRV = nullptr;
    ID3D11ShaderResourceView* SpotLightIndexBufferSRV = nullptr;

    static constexpr uint32 MAX_NUM_POINTLIGHTS = 50000;
    static constexpr uint32 MAX_NUM_SPOTLIGHTS = 50000;
    static constexpr uint32 MAX_TILE = 10000;
};
