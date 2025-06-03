#pragma once
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "EngineBaseTypes.h"

#include "Core/HAL/PlatformType.h"
#include "Core/Math/Vector4.h"

class FEditorViewportClient;

enum class ESamplerType : uint8
{
    Point,
    Linear,
};

class FGraphicsDevice
{
public:
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
    
    IDXGISwapChain* SwapChain = nullptr;
    
    ID3D11Texture2D* BackBufferTexture = nullptr;
    ID3D11RenderTargetView* BackBufferRTV = nullptr;
    
    ID3D11RasterizerState* RasterizerSolidBack = nullptr;
    ID3D11RasterizerState* RasterizerSolidFront = nullptr;
    ID3D11RasterizerState* RasterizerWireframeBack = nullptr;
    ID3D11RasterizerState* RasterizerShadow = nullptr;

    ID3D11DepthStencilState* DepthStencilState_Default = nullptr;
    ID3D11DepthStencilState* DepthStencilState_DepthWriteDisabled = nullptr;
    
    ID3D11BlendState* BlendState_AlphaBlend = nullptr;

    ID3D11SamplerState* SamplerState_LinearWrap = nullptr;
    ID3D11SamplerState* SamplerState_LinearClamp = nullptr;
    ID3D11SamplerState* SamplerState_PointWrap = nullptr;
    ID3D11SamplerState* SamplerState_PointClamp = nullptr;
    
    DXGI_SWAP_CHAIN_DESC SwapchainDesc;
    
    UINT ScreenWidth = 0;
    UINT ScreenHeight = 0;

    D3D11_VIEWPORT Viewport;
    
    FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // 화면을 초기화(clear) 할 때 사용할 색상(RGBA)

    void Initialize(HWND hWindow);
    
    void ChangeRasterizer(EViewModeIndex ViewModeIndex);
    
    ID3D11Texture2D* CreateTexture2D(const D3D11_TEXTURE2D_DESC& Description, const void* InitialData);

    void Release();
    
    void Prepare();

    void SwapBuffer() const;
    
    void Resize(HWND hWindow);
    
    ID3D11RasterizerState* GetCurrentRasterizer() const { return CurrentRasterizer; }

    ID3D11SamplerState* GetSamplerState(ESamplerType SamplerType) const;
    
    void SetFullScreen(HWND hWnd, bool bFullScreen);
    /*
    uint32 GetPixelUUID(POINT pt) const;
    uint32 DecodeUUIDColor(FVector4 UUIDColor) const;
    */
    
private:
    void CreateDeviceAndSwapChain(HWND hWindow);
    void CreateBackBuffer();
    void CreateDepthStencilState();
    void CreateRasterizerState();
    void CreateAlphaBlendState();
    void CreateSamplerState();
    
    void ReleaseDeviceAndSwapChain();
    void ReleaseFrameBuffer();
    void ReleaseRasterizerState();
    void ReleaseDepthStencilResources();
    void ReleaseBlendState();
    void ReleaseSamplerState();
    
    ID3D11RasterizerState* CurrentRasterizer = nullptr;

    const DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT BackBufferRTVFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

    bool bIsFullScreen = false; // 전체 화면 전환 전 상태를 저장하기 위한 변수
    RECT BeforeFullScreen = { 0, 0, 0, 0 }; // 전체 화면 전환 전의 윈도우 위치와 크기를 저장하기 위한 변수
};

