#pragma once
#include <memory>
#include "Texture.h"
#include "Container/Map.h"

class FRenderer;
class FGraphicsDevice;

class FResourceManager
{
public:
    void Initialize(FRenderer* Renderer, FGraphicsDevice* Device);
    void Release(FRenderer* Renderer);
    
    HRESULT LoadTextureFromFile(ID3D11Device* Device, const wchar_t* Filename, bool bIsSRGB = true);
    HRESULT LoadTextureFromDDS(ID3D11Device* Device, ID3D11DeviceContext* Context, const wchar_t* Filename);

    std::shared_ptr<FTexture> GetTexture(const FWString& Name) const;
    const TMap<FWString, std::shared_ptr<FTexture>>& GetTextureMap() const;

    
private:
    TMap<FWString, std::shared_ptr<FTexture>> TextureMap;
};
