#include "ShadowManager.h"

#include <utility>

#include "Components/Light/DirectionalLightComponent.h"
#include "Math/JungleMath.h"
#include "UnrealEd/EditorViewportClient.h"
#include "D3D11RHI/DXDBufferManager.h"

// --- 생성자 및 소멸자 ---

FShadowManager::FShadowManager()
{
    D3DDevice = nullptr;
    D3DContext = nullptr;
    ShadowSamplerCmp = nullptr;
    ShadowPointSampler = nullptr; // <<< 초기화 추가
    SpotShadowDepthRHI = nullptr;
    PointShadowCubeMapRHI = nullptr; // <<< 초기화 추가
    DirectionalShadowCascadeDepthRHI = nullptr;
}

FShadowManager::~FShadowManager()
{
    // 소멸 시 자동으로 리소스 해제 호출
    Release();
}



// --- Public 멤버 함수 구현 ---


bool FShadowManager::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager,
    uint32 InMaxSpotShadows, uint32 InSpotResolution,
    uint32 InMaxPointShadows, uint32 InPointResolution, uint32 InNumCascades, uint32 InDirResolution)
{
    if (D3DDevice) // 이미 초기화된 경우 방지
    {
        Release();
    }

    if (!InGraphics || !InGraphics->Device || !InGraphics->DeviceContext)
    {
        // UE_LOG(LogTemp, Error, TEXT("FShadowManager::Initialize: Invalid GraphicsDevice provided."));
        return false;
    }

    D3DDevice = InGraphics->Device;
    D3DContext = InGraphics->DeviceContext;
    BufferManager = InBufferManager;

    // RHI 구조체 할당
    SpotShadowDepthRHI = new FShadowDepthRHI();
    PointShadowCubeMapRHI = new FShadowCubeMapArrayRHI(); // << 추가
    DirectionalShadowCascadeDepthRHI = new FShadowDepthRHI();

    // 설정 값 저장
    MaxSpotLightShadows = InMaxSpotShadows;
                           

    MaxPointLightShadows = InMaxPointShadows; // << 추가
    //NumCascades = InNumCascades; // 차후 명시적인 바인딩 위해 주석처리 

    SpotShadowDepthRHI->ShadowMapResolution = InSpotResolution;
    PointShadowCubeMapRHI->ShadowMapResolution = InPointResolution; // << 추가
    DirectionalShadowCascadeDepthRHI->ShadowMapResolution = InDirResolution;

    // 리소스 생성 시도
    if (!CreateSpotShadowResources())
    {
        // UE_LOG(LogTemp, Error, TEXT("Failed to create spot shadow resources!"));
        Release();
        return false;
    }
    if (!CreatePointShadowResources()) // << 추가된 호출
    {
        // UE_LOG(LogTemp, Error, TEXT("Failed to create point shadow resources!"));
        Release();
        return false;
    }
    if (!CreateDirectionalShadowResources())
    {
        // UE_LOG(LogTemp, Error, TEXT("Failed to create directional shadow resources!"));
        Release();
        return false;
    }
    if (!CreateSamplers())
    {
        // UE_LOG(LogTemp, Error, TEXT("Failed to create shadow samplers!"));
        Release();
        return false;
    }

    // 방향성 광원 ViewProj 행렬 배열 크기 설정
    CascadesViewProjMatrices.SetNum(NumCascades);

    // UE_LOG(LogTemp, Log, TEXT("FShadowManager Initialized Successfully."));
    return true;
}

void FShadowManager::Release()
{
    // 생성된 역순 또는 그룹별로 리소스 해제
    ReleaseSamplers();
    ReleaseDirectionalShadowResources();
    ReleasePointShadowResources(); // << 추가
    ReleaseSpotShadowResources();

    // 배열 클리어
    CascadesViewProjMatrices.Empty();

    // D3D 객체 포인터는 외부에서 관리하므로 여기서는 nullptr 처리만 함
    D3DDevice = nullptr;
    D3DContext = nullptr;
}

void FShadowManager::BeginSpotShadowPass(uint32 SliceIndex)
{
    // 유효성 검사
    if (!D3DContext || std::cmp_greater_equal(SliceIndex, SpotShadowDepthRHI->ShadowDSVs.Num()) || !SpotShadowDepthRHI->ShadowDSVs[SliceIndex])
    {
        // UE_LOG(LogTemp, Warning, TEXT("BeginSpotShadowPass: Invalid slice index or DSV."));
        return;
    }

    // 렌더 타겟 설정 (DSV만 설정)
    ID3D11RenderTargetView* NullRTV = nullptr;
    D3DContext->OMSetRenderTargets(1, &NullRTV, SpotShadowDepthRHI->ShadowDSVs[SliceIndex]);

    // 뷰포트 설정
    D3D11_VIEWPORT Viewport;
    Viewport.Width = static_cast<float>(SpotShadowDepthRHI->ShadowMapResolution);
    Viewport.Height = static_cast<float>(SpotShadowDepthRHI->ShadowMapResolution);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    D3DContext->RSSetViewports(1, &Viewport);

    // DSV 클리어
    D3DContext->ClearDepthStencilView(SpotShadowDepthRHI->ShadowDSVs[SliceIndex], D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void FShadowManager::BeginPointShadowPass(uint32 SliceIndex)
{
    if (!D3DContext || !PointShadowCubeMapRHI || std::cmp_greater_equal(SliceIndex, PointShadowCubeMapRHI->ShadowDSVs.Num()) || !PointShadowCubeMapRHI->ShadowDSVs[SliceIndex])
    {
        // UE_LOG(LogTemp, Warning, TEXT("BeginPointShadowPass: Invalid slice index (%u) or DSV."), sliceIndex);
        return; // 유효성 검사
    }

    // 포인트 라이트의 DSV 바인딩 (이 DSV는 TextureCubeArray의 특정 슬라이스(큐브맵)를 가리킴)
    ID3D11RenderTargetView* NullRTV = nullptr;
    D3DContext->OMSetRenderTargets(1, &NullRTV, PointShadowCubeMapRHI->ShadowDSVs[SliceIndex]);

    // 뷰포트 설정 (큐브맵 한 면의 해상도)
    D3D11_VIEWPORT Viewport = {};
    Viewport.Width = static_cast<float>(PointShadowCubeMapRHI->ShadowMapResolution);
    Viewport.Height = static_cast<float>(PointShadowCubeMapRHI->ShadowMapResolution);
    Viewport.MinDepth = 0.0f; Viewport.MaxDepth = 1.0f;
    D3DContext->RSSetViewports(1, &Viewport);

    // DSV 클리어 (바인딩된 큐브맵 슬라이스의 모든 면을 클리어)
    D3DContext->ClearDepthStencilView(PointShadowCubeMapRHI->ShadowDSVs[SliceIndex], D3D11_CLEAR_DEPTH, 1.0f, 0);
}


void FShadowManager::BeginDirectionalShadowCascadePass(uint32 CascadeIndex)
{
    // 유효성 검사
    if (!D3DContext || std::cmp_greater_equal(CascadeIndex, DirectionalShadowCascadeDepthRHI->ShadowDSVs.Num()) || !DirectionalShadowCascadeDepthRHI->ShadowDSVs[CascadeIndex])
    {
         UE_LOG(ELogLevel::Warning, TEXT("BeginDirectionalShadowCascadePass: Invalid cascade index or DSV."));
        return;
    }

    // 렌더 타겟 설정 (DSV만 설정)
    ID3D11RenderTargetView* NullRTV = nullptr;
    D3DContext->OMSetRenderTargets(1, &NullRTV, DirectionalShadowCascadeDepthRHI->ShadowDSVs[CascadeIndex]);

    // 뷰포트 설정
    D3D11_VIEWPORT Viewport;
    Viewport.Width = static_cast<float>(DirectionalShadowCascadeDepthRHI->ShadowMapResolution);
    Viewport.Height = static_cast<float>(DirectionalShadowCascadeDepthRHI->ShadowMapResolution);
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    D3DContext->RSSetViewports(1, &Viewport);

    // DSV 클리어
    D3DContext->ClearDepthStencilView(DirectionalShadowCascadeDepthRHI->ShadowDSVs[CascadeIndex], D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void FShadowManager::BindResourcesForSampling(
    uint32 SpotShadowSlot, uint32 PointShadowSlot, uint32 DirectionalShadowSlot, // << pointShadowSlot 추가
    uint32 SamplerCmpSlot, uint32 SamplerPointSlot)
{
    if (!D3DContext)
    {
        return;
    }

    // SRV 바인딩
    if (SpotShadowDepthRHI && SpotShadowDepthRHI->ShadowSRV)
    {
        D3DContext->PSSetShaderResources(SpotShadowSlot, 1, &SpotShadowDepthRHI->ShadowSRV);
    }
    if (PointShadowCubeMapRHI && PointShadowCubeMapRHI->ShadowSRV) // << 추가
    {
        D3DContext->PSSetShaderResources(PointShadowSlot, 1, &PointShadowCubeMapRHI->ShadowSRV);
    }
    if (DirectionalShadowCascadeDepthRHI && DirectionalShadowCascadeDepthRHI->ShadowSRV)
    {
        D3DContext->PSSetShaderResources(DirectionalShadowSlot, 1, &DirectionalShadowCascadeDepthRHI->ShadowSRV);

        FCascadeConstantBuffer CascadeData = {};
        CascadeData.World = FMatrix::Identity;
        for (int32 i = 0; i < NumCascades; i++)
        {
            CascadeData.ViewProj[i] = CascadesViewProjMatrices[i];
            CascadeData.InvViewProj[i] = FMatrix::Inverse(CascadeData.ViewProj[i]);
            if (i >= CascadesInvProjMatrices.Num()) { continue; }
            CascadeData.InvProj[i] = CascadesInvProjMatrices[i];
        }

        if (CascadeSplits.Num() >= 4) {
            CascadeData.CascadeSplit = { CascadeSplits[0], CascadeSplits[1], CascadeSplits[2], CascadeSplits[3] };
        }
            //CascadeData.CascadeSplits[i] = CascadeSplits[i];
        //CascadeData.CascadeSplits[NumCascades] = CascadeSplits[NumCascades];

        BufferManager->UpdateConstantBuffer(TEXT("FCascadeConstantBuffer"), CascadeData);
        BufferManager->BindConstantBuffer(TEXT("FCascadeConstantBuffer"), 9, EShaderStage::Pixel);
    }

    // 샘플러 바인딩
    if (ShadowSamplerCmp)
    {
        D3DContext->PSSetSamplers(SamplerCmpSlot, 1, &ShadowSamplerCmp);
    }
    if (ShadowPointSampler)
    {
        D3DContext->PSSetSamplers(SamplerPointSlot, 1, &ShadowPointSampler);
    }
}

FMatrix FShadowManager::GetCascadeViewProjMatrix(int Idx) const
{
    if (Idx < 0 || Idx >= CascadesViewProjMatrices.Num())
    {
        UE_LOG(ELogLevel::Warning, TEXT("GetCascadeViewProjMatrix: Invalid cascade index."));
        return FMatrix::Identity;
    }
    return CascadesViewProjMatrices[Idx];
}


// --- Private 멤버 함수 구현 (리소스 생성/해제 헬퍼) ---

bool FShadowManager::CreateSpotShadowResources()
{
    // 유효성 검사
    if (!D3DDevice || MaxSpotLightShadows == 0 || SpotShadowDepthRHI->ShadowMapResolution == 0)
    {
        return false;
    }

    // 1. Texture2DArray 생성
    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = SpotShadowDepthRHI->ShadowMapResolution;
    TexDesc.Height = SpotShadowDepthRHI->ShadowMapResolution;
    TexDesc.MipLevels = 1;
    TexDesc.ArraySize = MaxSpotLightShadows;
    TexDesc.Format = DXGI_FORMAT_R32_TYPELESS; // 깊이 포맷
    TexDesc.SampleDesc.Count = 1;
    TexDesc.SampleDesc.Quality = 0;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    TexDesc.CPUAccessFlags = 0;
    TexDesc.MiscFlags = 0;

    HRESULT hr = D3DDevice->CreateTexture2D(&TexDesc, nullptr, &SpotShadowDepthRHI->ShadowTexture);
    if (FAILED(hr))
    {
        // UE_LOG(LogTemp, Error, TEXT("CreateTexture2D failed for SpotShadowArrayTexture (HR=0x%X)"), hr);
        return false;
    }

    // 2. 전체 배열용 SRV 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 읽기용 포맷
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    SrvDesc.Texture2DArray.MostDetailedMip = 0;
    SrvDesc.Texture2DArray.MipLevels = 1;
    SrvDesc.Texture2DArray.FirstArraySlice = 0;
    SrvDesc.Texture2DArray.ArraySize = MaxSpotLightShadows;

    hr = D3DDevice->CreateShaderResourceView(SpotShadowDepthRHI->ShadowTexture, &SrvDesc, &SpotShadowDepthRHI->ShadowSRV);
    if (FAILED(hr))
    {
        // UE_LOG(LogTemp, Error, TEXT("CreateShaderResourceView failed for SpotShadowArraySRV (HR=0x%X)"), hr);
        ReleaseSpotShadowResources(); // 생성된 텍스처 정리
        return false;
    }

    
    SpotShadowDepthRHI->ShadowDSVs.SetNum(MaxSpotLightShadows);
    for (uint32 Idx = 0; Idx < MaxSpotLightShadows; ++Idx)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
        DsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 깊이 포맷
        DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        DsvDesc.Texture2DArray.MipSlice = 0;
        DsvDesc.Texture2DArray.FirstArraySlice = Idx;
        DsvDesc.Texture2DArray.ArraySize = 1;
        DsvDesc.Flags = 0;

        hr = D3DDevice->CreateDepthStencilView(SpotShadowDepthRHI->ShadowTexture, &DsvDesc, &SpotShadowDepthRHI->ShadowDSVs[Idx]);
        if (FAILED(hr))
        {
            // UE_LOG(LogTemp, Error, TEXT("CreateDepthStencilView failed for SpotShadowSliceDSV[%u] (HR=0x%X)"), i, hr);
            ReleaseSpotShadowResources(); // 생성된 것들 정리
            return false;
        }
    }

    SpotShadowDepthRHI->ShadowSRVs.SetNum(MaxSpotLightShadows);
    for (uint32 Idx = 0; Idx < MaxSpotLightShadows; ++Idx)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
        SrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        SrvDesc.Texture2DArray.MostDetailedMip = 0;
        SrvDesc.Texture2DArray.MipLevels = 1;
        SrvDesc.Texture2DArray.FirstArraySlice = Idx;
        SrvDesc.Texture2DArray.ArraySize = 1;

        hr = D3DDevice->CreateShaderResourceView(SpotShadowDepthRHI->ShadowTexture, &SrvDesc, & SpotShadowDepthRHI->ShadowSRVs[Idx]);
        if (FAILED(hr)) { ReleaseDirectionalShadowResources(); return false; }
    }

    return true;
}

void FShadowManager::ReleaseSpotShadowResources()
{
    if (SpotShadowDepthRHI)
    {
        SpotShadowDepthRHI->Release();
        delete SpotShadowDepthRHI;
        SpotShadowDepthRHI = nullptr;
    }
}

bool FShadowManager::CreatePointShadowResources() // << 추가된 함수 구현
{
    if (!D3DDevice || !PointShadowCubeMapRHI || MaxPointLightShadows == 0 || PointShadowCubeMapRHI->ShadowMapResolution == 0)
    {
        return false;
    }

    // 1. TextureCubeArray 생성 (Texture2D로 생성 후 MiscFlags 사용)
    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = PointShadowCubeMapRHI->ShadowMapResolution;
    TexDesc.Height = PointShadowCubeMapRHI->ShadowMapResolution;
    TexDesc.MipLevels = 1;
    TexDesc.ArraySize = MaxPointLightShadows * 6; // <<< 총 면의 개수 (큐브맵 개수 * 6)
    TexDesc.Format = DXGI_FORMAT_R32_TYPELESS;     // TYPELESS 사용
    TexDesc.SampleDesc.Count = 1;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    TexDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE; // <<< 큐브맵임을 명시

    HRESULT hr = D3DDevice->CreateTexture2D(&TexDesc, nullptr, &PointShadowCubeMapRHI->ShadowTexture);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Shadow Cube Map texture!"));
        return hr;
    }

    // 2. 전체 배열용 SRV 생성 (TEXTURECUBEARRAY 사용)
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 읽기 형식
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY; // <<< 큐브맵 배열 뷰
    SrvDesc.TextureCubeArray.MostDetailedMip = 0;
    SrvDesc.TextureCubeArray.MipLevels = 1;
    SrvDesc.TextureCubeArray.First2DArrayFace = 0;      // 첫 번째 큐브맵의 인덱스
    SrvDesc.TextureCubeArray.NumCubes = MaxPointLightShadows; // <<< 총 큐브맵 개수

    hr = D3DDevice->CreateShaderResourceView(PointShadowCubeMapRHI->ShadowTexture, &SrvDesc, &PointShadowCubeMapRHI->ShadowSRV);
    if (FAILED(hr))
    {
        // UE_LOG(LogTemp, Error, TEXT("CreateShaderResourceView failed for PointShadowCubeMap SRV (HR=0x%X)"), hr);
        ReleasePointShadowResources();
        return false;
    }

    // 3. 각 큐브맵 슬라이스용 DSV 생성 (렌더링 시 GS의 SV_RenderTargetArrayIndex 사용)
    PointShadowCubeMapRHI->ShadowDSVs.SetNum(MaxPointLightShadows);
    for (uint32 Idx = 0; Idx < MaxPointLightShadows; ++Idx)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
        DsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // 쓰기 형식
        DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY; // DSV는 2D Array View 사용
        DsvDesc.Texture2DArray.MipSlice = 0;
        DsvDesc.Texture2DArray.FirstArraySlice = Idx * 6; // <<< 각 큐브맵의 시작 인덱스 (i번째 큐브맵의 +X 면)
        DsvDesc.Texture2DArray.ArraySize = 6;       // <<< 각 DSV는 큐브맵 하나(6면)를 가리킴
        DsvDesc.Flags = 0;

        hr = D3DDevice->CreateDepthStencilView(PointShadowCubeMapRHI->ShadowTexture, &DsvDesc, &PointShadowCubeMapRHI->ShadowDSVs[Idx]);
        if (FAILED(hr))
        {
            // UE_LOG(LogTemp, Error, TEXT("CreateDepthStencilView failed for PointShadowCubeMap DSV[%u] (HR=0x%X)"), i, hr);
            ReleasePointShadowResources();
            return false;
        }
    }

    // --- 4. ImGui 디버그용: 각 면에 대한 개별 SRV 생성 ---
    PointShadowCubeMapRHI->ShadowFaceSRVs.SetNum(MaxPointLightShadows); // 외부 배열 크기 설정
    for (uint32 PointLightIdx = 0; PointLightIdx < MaxPointLightShadows; ++PointLightIdx) // 각 포인트 라이트 루프
    {
        PointShadowCubeMapRHI->ShadowFaceSRVs[PointLightIdx].SetNum(6); // 내부 배열 크기 설정 (6개 면)
        for (uint32 FaceIdx = 0; FaceIdx < 6; ++FaceIdx) // 각 면 루프 (+X, -X, +Y, -Y, +Z, -Z 순서 가정)
        {
            // 이 면의 플랫 배열 인덱스 계산
            uint32 FlatArrayIndex = PointLightIdx * 6 + FaceIdx;

            D3D11_SHADER_RESOURCE_VIEW_DESC FaceSrvDesc = {};
            FaceSrvDesc.Format = DXGI_FORMAT_R32_FLOAT; // 읽기 형식
            FaceSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY; // 소스는 배열이지만, 뷰는 단일 슬라이스
            FaceSrvDesc.Texture2DArray.MostDetailedMip = 0;
            FaceSrvDesc.Texture2DArray.MipLevels = 1;
            FaceSrvDesc.Texture2DArray.FirstArraySlice = FlatArrayIndex; // <<< 이 면의 인덱스 지정
            FaceSrvDesc.Texture2DArray.ArraySize = 1; // <<< SRV는 단 1개의 슬라이스만 가리킴

            hr = D3DDevice->CreateShaderResourceView(PointShadowCubeMapRHI->ShadowTexture, &FaceSrvDesc, &PointShadowCubeMapRHI->ShadowFaceSRVs[PointLightIdx][FaceIdx]);
            if (FAILED(hr))
            {
                // UE_LOG(LogTemp, Error, TEXT("CreateShaderResourceView failed for PointShadow Face SRV [%u][%u] (HR=0x%X)"), i, j, hr);
                // 실패 시, 지금까지 생성된 모든 리소스 정리 필요 (더 복잡한 롤백 로직 또는 단순하게 전체 해제)
                ReleasePointShadowResources();
                return false;
            }
        }
    }

    return true;
}

void FShadowManager::ReleasePointShadowResources() // << 추가된 함수 구현
{
    if (PointShadowCubeMapRHI)
    {
        PointShadowCubeMapRHI->Release();
        delete PointShadowCubeMapRHI; // Initialize에서 new 했으므로 delete 필요
        PointShadowCubeMapRHI = nullptr;
    }
}

bool FShadowManager::CreateDirectionalShadowResources()
{
    // 유효성 검사
    if (!D3DDevice || NumCascades == 0 || DirectionalShadowCascadeDepthRHI->ShadowMapResolution == 0)
    {
        return false;
    }

    // 1. Texture2DArray 생성 (CSM 용)
    D3D11_TEXTURE2D_DESC TexDesc = {};
    TexDesc.Width = DirectionalShadowCascadeDepthRHI->ShadowMapResolution;
    TexDesc.Height = DirectionalShadowCascadeDepthRHI->ShadowMapResolution;
    TexDesc.MipLevels = 1;
    TexDesc.ArraySize = NumCascades; // 캐스케이드 개수만큼
    TexDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    TexDesc.SampleDesc.Count = 1;
    TexDesc.SampleDesc.Quality = 0;
    TexDesc.Usage = D3D11_USAGE_DEFAULT;
    TexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = D3DDevice->CreateTexture2D(&TexDesc, nullptr, &DirectionalShadowCascadeDepthRHI->ShadowTexture);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create Shadow Cube Map texture!"));
        return hr;
    }

    // 2. 전체 배열용 SRV 생성
    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    SrvDesc.Texture2DArray.MostDetailedMip = 0;
    SrvDesc.Texture2DArray.MipLevels = 1;
    SrvDesc.Texture2DArray.FirstArraySlice = 0;
    SrvDesc.Texture2DArray.ArraySize = NumCascades;

    hr = D3DDevice->CreateShaderResourceView(DirectionalShadowCascadeDepthRHI->ShadowTexture, &SrvDesc, &DirectionalShadowCascadeDepthRHI->ShadowSRV);
    if (FAILED(hr)) { ReleaseDirectionalShadowResources(); return false; }

    // 3. 각 캐스케이드용 DSV 생성
    DirectionalShadowCascadeDepthRHI->ShadowDSVs.SetNum(1);

    D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
    DsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
    DsvDesc.Texture2DArray.MipSlice = 0;
    DsvDesc.Texture2DArray.FirstArraySlice = 0;
    DsvDesc.Texture2DArray.ArraySize = NumCascades;

    hr = D3DDevice->CreateDepthStencilView(DirectionalShadowCascadeDepthRHI->ShadowTexture, &DsvDesc, &DirectionalShadowCascadeDepthRHI->ShadowDSVs[0]);
    if (FAILED(hr)) { ReleaseDirectionalShadowResources(); return false; }
    /*DirectionalShadowCascadeDepthRHI->ShadowDSVs.SetNum(NumCascades);
    for (uint32 i = 0; i < NumCascades; ++i)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = 0;
        dsvDesc.Texture2DArray.FirstArraySlice = i;
        dsvDesc.Texture2DArray.ArraySize = 1;

        hr = D3DDevice->CreateDepthStencilView(DirectionalShadowCascadeDepthRHI->ShadowTexture, &dsvDesc, &DirectionalShadowCascadeDepthRHI->ShadowDSVs[i]);
        if (FAILED(hr)) { ReleaseDirectionalShadowResources(); return false; }
    }*/

    // Directional Light의 Shadow Map 개수 = Cascade 개수 (분할 개수)
    DirectionalShadowCascadeDepthRHI->ShadowSRVs.SetNum(NumCascades); 
    for (uint32 Idx = 0; Idx < NumCascades; ++Idx)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC LocalSrvDesc = {};
        LocalSrvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        LocalSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        LocalSrvDesc.Texture2DArray.MostDetailedMip = 0;
        LocalSrvDesc.Texture2DArray.MipLevels = 1;
        LocalSrvDesc.Texture2DArray.FirstArraySlice = Idx;
        LocalSrvDesc.Texture2DArray.ArraySize = 1;

        hr = D3DDevice->CreateShaderResourceView(DirectionalShadowCascadeDepthRHI->ShadowTexture, &LocalSrvDesc, & DirectionalShadowCascadeDepthRHI->ShadowSRVs[Idx]);
        if (FAILED(hr)) { ReleaseDirectionalShadowResources(); return false; }
    }
    
    
    return true;
}

void FShadowManager::ReleaseDirectionalShadowResources()
{
    if (DirectionalShadowCascadeDepthRHI)
    {
        DirectionalShadowCascadeDepthRHI->Release();
        delete DirectionalShadowCascadeDepthRHI; // Initialize에서 new 했으므로 delete 필요
        DirectionalShadowCascadeDepthRHI = nullptr;
    }
}

void FShadowManager::UpdateCascadeMatrices(const std::shared_ptr<FEditorViewportClient>& Viewport, UDirectionalLightComponent* DirectionalLight)
{    
    FMatrix InvViewProj = FMatrix::Inverse(Viewport->GetViewMatrix()*Viewport->GetProjectionMatrix());

    CascadesViewProjMatrices.Empty();
    CascadesInvProjMatrices.Empty();
    const FMatrix CamView = Viewport->GetViewMatrix();
    const float NearClip = Viewport->GetCameraNearClip();
    const float FarClip = Viewport->GetCameraFarClip();
    const float FOV = Viewport->GetCameraFOV();          // Degrees
    const float AspectRatio = Viewport->AspectRatio;

    const float HalfHFOV = FMath::DegreesToRadians(FOV) * 0.5f;
    const float TanHFOV = FMath::Tan(HalfHFOV);
    const float TanVFOV = TanHFOV / AspectRatio;
    const FMatrix InvView = FMatrix::Inverse(CamView);

    //CascadeSplits.Empty();
    CascadeSplits.SetNum(NumCascades + 1);
    CascadeSplits[0] = NearClip;
    CascadeSplits[NumCascades] = FarClip;
    for (uint32 Idx = 1; Idx < NumCascades; ++Idx)
    {
        const float P = static_cast<float>(Idx) / static_cast<float>(NumCascades);
        const float LogSplit = NearClip * powf(FarClip / NearClip, P);      // 로그 분포
        const float UniSplit = NearClip + (FarClip - NearClip) * P;         // 균등 분포
        CascadeSplits[Idx] = 0.7f * LogSplit + 0.3f * UniSplit;         // 혼합 (0.5:0.5)
    }

    // 4) LightDir, Up
    const FVector LightDir = DirectionalLight->GetDirection().GetSafeNormal();
    FVector Up = FVector::UpVector;
    if (FMath::Abs(FVector::DotProduct(LightDir, FVector::UpVector)) > 0.99f)
    {
        Up = FVector::ForwardVector;
    }

    CascadesViewProjMatrices.Empty();

    for (uint32 c = 0; c< NumCascades; ++c)
    {
    	// i 단계의 Near / Far (월드 단위) 계산
		float splitN = CascadeSplits[c];
		float splitF = CascadeSplits[c + 1];
		float zn = (splitN - NearClip) / (FarClip - NearClip);
        float zf = (splitF - NearClip) / (FarClip - NearClip);

		// 뷰 공간 평면상의 X,Y 절댓값
		float nx = TanHFOV * splitN;
		float ny = TanVFOV * splitN;
		float fx = TanHFOV * splitF;
		float fy = TanVFOV * splitF;

		// 뷰 공간 8개 코너
		const FVector ViewCorners[8] = {
			{ -nx,  ny, splitN },
			{  nx,  ny, splitN },
			{  nx, -ny, splitN },
			{ -nx, -ny, splitN },
			{ -fx,  fy, splitF },
			{  fx,  fy, splitF },
			{  fx, -fy, splitF },
			{ -fx, -fy, splitF }
		};

		// 월드 공간으로 변환
		FVector WorldCorners[8];
		for (int Idx = 0; Idx < 8; ++Idx)
		{
			// TransformPosition 은 w=1 가정 + divide 처리
			WorldCorners[Idx] = InvView.TransformPosition(ViewCorners[Idx]);
		}

        // Light Space AABB
        FMatrix LightView = DirectionalLight->GetViewMatrix();
		LightView.M[3][0] = LightView.M[3][1] = LightView.M[3][2] = 0; // translation 제거 - Rot만 필요

        FVector Min(FLT_MAX), Max(-FLT_MAX);
		for (const auto& World : WorldCorners) {
			Min.X = FMath::Min(Min.X, World.X);  Max.X = FMath::Max(Max.X, World.X);
			Min.Y = FMath::Min(Min.Y, World.Y);  Max.Y = FMath::Max(Max.Y, World.Y);
			Min.Z = FMath::Min(Min.Z, World.Z);  Max.Z = FMath::Max(Max.Z, World.Z);
		}
		FVector CenterWS = (Min + Max) * 0.5f;
		const float Radius = FMath::Max3(Max.X - Min.X, Max.Y - Min.Y, Max.Z - Min.Z) * 0.5f;

		// 3. Light View 생성
		const FVector Eye = CenterWS - LightDir * Radius;
		LightView = JungleMath::CreateViewMatrix(Eye, CenterWS, Up);

		// 4. 모든 WorldCorners를 LightView로 변환, LightSpace에서의 Min/Max 구함
		FVector MinLS(FLT_MAX), MaxLS(-FLT_MAX);
		for (auto& World : WorldCorners) {
			const FVector LS = LightView.TransformPosition(World);
			MinLS.X = FMath::Min(MinLS.X, LS.X);  MaxLS.X = FMath::Max(MaxLS.X, LS.X);
			MinLS.Y = FMath::Min(MinLS.Y, LS.Y);  MaxLS.Y = FMath::Max(MaxLS.Y, LS.Y);
			MinLS.Z = FMath::Min(MinLS.Z, LS.Z);  MaxLS.Z = FMath::Max(MaxLS.Z, LS.Z);
		}
		const float Zm = (MaxLS.Z - MinLS.Z) * 0.1f;
        MinLS.X -= Zm; MinLS.Y -= Zm; MinLS.Z -= Zm;
        MaxLS.X -= Zm; MaxLS.Y -= Zm; MaxLS.Z -= Zm;

		// 5. LightSpace에서 Ortho 행렬 생성
		FMatrix LightProj = JungleMath::CreateOrthoProjectionMatrix(
            MaxLS.X- MinLS.X,
            MaxLS.Y - MinLS.Y,
			MinLS.Z, MaxLS.Z
		);

		// 6. 최종 ViewProj 행렬
		CascadesViewProjMatrices.Add(LightView * LightProj);
        CascadesInvProjMatrices.Add(FMatrix::Inverse(LightProj));
    }

}

bool FShadowManager::CreateSamplers()
{
    if (!D3DDevice)
    {
        return false;
    }

    // 1. Comparison Sampler (PCF 용)
    D3D11_SAMPLER_DESC SampDescCmp = {};
    SampDescCmp.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT; // 선형 필터링 비교
    SampDescCmp.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    SampDescCmp.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    SampDescCmp.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    SampDescCmp.MipLODBias = 0.0f;
    SampDescCmp.MaxAnisotropy = 1;
    SampDescCmp.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL; // 깊이 비교 함수
    SampDescCmp.BorderColor[0] = 1.0f; // 경계 밖 = 깊이 최대 (그림자 없음)
    SampDescCmp.BorderColor[1] = 1.0f;
    SampDescCmp.BorderColor[2] = 1.0f;
    SampDescCmp.BorderColor[3] = 1.0f;
    SampDescCmp.MinLOD = 0;
    SampDescCmp.MaxLOD = D3D11_FLOAT32_MAX;

    HRESULT hr = D3DDevice->CreateSamplerState(&SampDescCmp, &ShadowSamplerCmp);
    if (FAILED(hr))
    {
        return false;
    }

    // 2. Point Sampler (하드 섀도우 또는 VSM/ESM 등)
    D3D11_SAMPLER_DESC PointSamplerDesc = {};
    PointSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    PointSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    PointSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    PointSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    PointSamplerDesc.BorderColor[0] = 1.0f; PointSamplerDesc.BorderColor[1] = 1.0f; PointSamplerDesc.BorderColor[2] = 1.0f; PointSamplerDesc.BorderColor[3] = 1.0f; // 높은 깊이 값
    PointSamplerDesc.MinLOD = 0;
    PointSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    PointSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER; // 비교 안 함

    hr = D3DDevice->CreateSamplerState(&PointSamplerDesc, &ShadowPointSampler);
    if (FAILED(hr))
    {
        // UE_LOG(ELogLevel::Error, TEXT("Failed to create Shadow Point Sampler!"));
        ReleaseSamplers(); // 생성된 Comparison 샘플러 해제
        return false;
    }

    return true;
}

void FShadowManager::ReleaseSamplers()
{
    if (ShadowSamplerCmp) { ShadowSamplerCmp->Release(); ShadowSamplerCmp = nullptr; }
    if (ShadowPointSampler) { ShadowPointSampler->Release(); ShadowPointSampler = nullptr; }
    //if (ShadowSampler) { ShadowSampler->Release(); ShadowSampler = nullptr; }
}
