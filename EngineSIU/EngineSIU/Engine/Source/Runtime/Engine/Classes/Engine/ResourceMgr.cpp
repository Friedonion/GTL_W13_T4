#include "ResourceMgr.h"
#include <fstream>
#include <ranges>
#include <unordered_map>
#include <wincodec.h>
#include "Define.h"
#include "Components/SkySphereComponent.h"
#include "D3D11RHI/GraphicDevice.h"
#include "DirectXTK/DDSTextureLoader.h"
#include "Engine/FObjLoader.h"


void FResourceManager::Initialize(FRenderer* Renderer, FGraphicsDevice* Device)
{
    LoadTextureFromDDS(Device->Device, Device->DeviceContext, L"Assets/Texture/font.dds");
    LoadTextureFromDDS(Device->Device, Device->DeviceContext, L"Assets/Texture/UUID_Font.dds");

    LoadTextureFromFile(Device->Device, L"Assets/Texture/ocean_sky.jpg");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/font.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/T_Explosion_SubUV.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/UUID_Font.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/spotLight.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/Aim.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/Noise.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/Title.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/StartButton.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/Title2.png");
    LoadTextureFromFile(Device->Device, L"Assets/Texture/TitleSubUV.png");


    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/S_Actor.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/S_LightSpot.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/S_LightPoint.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/S_LightDirectional.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/S_ExpoHeightFog.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/S_AtmosphericHeightFog.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Editor/Icon/AmbientLight_64x.png");
    
    LoadTextureFromFile(Device->Device, L"Assets/Viewer/Bone_16x.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Viewer/BoneNonWeighted_16x.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Viewer/GroupActor_16x.PNG");
    LoadTextureFromFile(Device->Device, L"Assets/Viewer/qer.PNG");
}

void FResourceManager::Release(FRenderer* Renderer)
{
    for (const auto& Pair : TextureMap)
    {
        FTexture* Texture = Pair.Value.get();
        Texture->Release();
    }
    TextureMap.Empty();
}

struct PairHash
{
    template <typename T1, typename T2>
    std::size_t operator()(const std::pair<T1, T2>& Pair) const
    {
        return std::hash<T1>()(Pair.first) ^ (std::hash<T2>()(Pair.second) << 1);
    }
};

struct TupleHash
{
    template <typename T1, typename T2, typename T3>
    std::size_t operator()(const std::tuple<T1, T2, T3>& Tuple) const
    {
        std::size_t h1 = std::hash<T1>()(std::get<0>(Tuple));
        std::size_t h2 = std::hash<T2>()(std::get<1>(Tuple));
        std::size_t h3 = std::hash<T3>()(std::get<2>(Tuple));

        return h1 ^ (h2 << 1) ^ (h3 << 2); // 해시 값 섞기
    }
};

std::shared_ptr<FTexture> FResourceManager::GetTexture(const FWString& Name) const
{
    auto* TempValue = TextureMap.Find(Name);
    return TempValue ? *TempValue : nullptr;
}

const TMap<FWString, std::shared_ptr<FTexture>>& FResourceManager::GetTextureMap() const
{
    return TextureMap;
}


HRESULT FResourceManager::LoadTextureFromFile(ID3D11Device* Device, const wchar_t* Filename, bool bIsSRGB)
{
    IWICImagingFactory* WicFactory = nullptr;
    IWICBitmapDecoder* Decoder = nullptr;
    IWICBitmapFrameDecode* Frame = nullptr;
    IWICFormatConverter* Converter = nullptr;

    // WIC 팩토리 생성
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WicFactory));
    if (FAILED(hr))
    {
        return hr;
    }


    // 이미지 파일 디코딩
    hr = WicFactory->CreateDecoderFromFilename(Filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &Decoder);
    if (FAILED(hr))
    {
        return hr;
    }


    hr = Decoder->GetFrame(0, &Frame);
    if (FAILED(hr))
    {
        return hr;
    }

    // WIC 포맷 변환기 생성 (픽셀 포맷 변환)
    hr = WicFactory->CreateFormatConverter(&Converter);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = Converter->Initialize(Frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        return hr;
    }

    // 이미지 크기 가져오기
    UINT Width, Height;
    Frame->GetSize(&Width, &Height);

    // 픽셀 데이터 로드
    BYTE* ImageData = new BYTE[Width * Height * 4];
    hr = Converter->CopyPixels(nullptr, Width * 4, Width * Height * 4, ImageData);
    if (FAILED(hr)) {
        delete[] ImageData;
        return hr;
    }

    // DirectX 11 텍스처 생성
    D3D11_TEXTURE2D_DESC TextureDesc = {};
    TextureDesc.Width = Width;
    TextureDesc.Height = Height;
    TextureDesc.MipLevels = 1;
    TextureDesc.ArraySize = 1;
    TextureDesc.Format = bIsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    TextureDesc.SampleDesc.Count = 1;
    TextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = ImageData;
    InitData.SysMemPitch = Width * 4;
    ID3D11Texture2D* Texture2D;
    hr = Device->CreateTexture2D(&TextureDesc, &InitData, &Texture2D);
    delete[] ImageData;
    if (FAILED(hr))
    {
        return hr;
    }

    // Shader Resource View 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = TextureDesc.Format;
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SrvDesc.Texture2D.MostDetailedMip = 0;
    SrvDesc.Texture2D.MipLevels = 1;
    ID3D11ShaderResourceView* TextureSrv;
    hr = Device->CreateShaderResourceView(Texture2D, &SrvDesc, &TextureSrv);

    // 리소스 해제
    WicFactory->Release();
    Decoder->Release();
    Frame->Release();
    Converter->Release();

    const FWString Name = FWString(Filename);
    TextureMap[Name] = std::make_shared<FTexture>(TextureSrv, Texture2D, ESamplerType::Linear, Name, Width, Height);

    FConsole::GetInstance().AddLog(ELogLevel::Warning, "Texture File Load Successs");
    return hr;
}

HRESULT FResourceManager::LoadTextureFromDDS(ID3D11Device* Device, ID3D11DeviceContext* Context, const wchar_t* Filename)
{

    ID3D11Resource* Texture = nullptr;
    ID3D11ShaderResourceView* TextureView = nullptr;

    HRESULT hr = DirectX::CreateDDSTextureFromFile(
        Device, Context,
        Filename,
        &Texture,
        &TextureView
    );
    if (FAILED(hr) || Texture == nullptr)
    {
        abort();
    }

    ID3D11Texture2D* Texture2D = nullptr;
    hr = Texture->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&Texture2D);
    if (FAILED(hr) || Texture2D == nullptr) {
        std::wcerr << L"Failed to query ID3D11Texture2D interface!" << std::endl;
        Texture->Release();
        abort();
        return hr;
    }

    D3D11_TEXTURE2D_DESC TexDesc;
    Texture2D->GetDesc(&TexDesc);
    uint32 Width = TexDesc.Width;
    uint32 Height = TexDesc.Height;

    const FWString Name = FWString(Filename);
    TextureMap[Name] = std::make_shared<FTexture>(TextureView, Texture2D, ESamplerType::Linear, Name, Width, Height);

    FConsole::GetInstance().AddLog(ELogLevel::Warning, "Texture File Load Successs");

    return hr;
}
