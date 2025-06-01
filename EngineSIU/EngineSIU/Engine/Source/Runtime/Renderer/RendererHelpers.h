#pragma once
#include "Launch/EngineLoop.h"

enum class EShaderSRVSlot : int8
{
    SRV_SpotLight = 50,
    SRV_DirectionalLight = 51,
    SRV_PointLight = 52,
    SRV_DepthOfField_LayerInfo = 90,
    SRV_DepthOfField_LayerNear = 91,
    SRV_DepthOfField_LayerFar = 92,
    SRV_DepthOfField_FilteredCoC = 93,
    SRV_DepthOfField_BlurredCoC = 94,
    SRV_SceneDepth = 99,
    SRV_Scene = 100,
    SRV_Translucent = 101,
    SRV_PostProcess = 102,
    SRV_Editor = 103,
    SRV_EditorOverlay = 104,
    SRV_Fog = 105,
    SRV_Debug = 106,
    SRV_CameraEffect = 107,
    SRV_Viewport = 120,

    SRV_MAX = 127,
};

namespace MaterialUtils
{
    inline void UpdateMaterial(FDXDBufferManager* BufferManager, FGraphicsDevice* Graphics, const FMaterialInfo& MaterialInfo)
    {
        FMaterialConstants Data;
        
        Data.TextureFlag = MaterialInfo.TextureFlag;
        
        Data.DiffuseColor = MaterialInfo.DiffuseColor;
        
        Data.SpecularColor = MaterialInfo.SpecularColor;
        Data.Shininess = MaterialInfo.Shininess;
        
        Data.EmissiveColor = MaterialInfo.EmissiveColor;
        Data.Transparency = MaterialInfo.Transparency;

        Data.Metallic = MaterialInfo.Metallic;
        Data.Roughness = MaterialInfo.Roughness;

        BufferManager->UpdateConstantBuffer(TEXT("FMaterialConstants"), Data);

        ID3D11ShaderResourceView* SRVs[9] = {};

        for (uint8 Idx = 0; Idx < static_cast<uint8>(EMaterialTextureSlots::MTS_MAX); ++Idx)
        {
            if (MaterialInfo.TextureFlag & (1 << Idx)) // EMaterialTextureFlags와 EMaterialTextureSlots의 순서가 일치한다는 전제 조건.
            {
                std::shared_ptr<FTexture> Texture = FEngineLoop::ResourceManager.GetTexture(MaterialInfo.TextureInfos[Idx].TexturePath);

                SRVs[Idx] = Texture->TextureSRV;

                if (Idx == static_cast<uint8>(EMaterialTextureSlots::MTS_Diffuse))
                {
                    // for Gouraud shading
                    Graphics->DeviceContext->VSSetShaderResources(0, 1, &SRVs[Idx]);
                }
            }
        }

        Graphics->DeviceContext->PSSetShaderResources(0, 9, SRVs);
    }
}
