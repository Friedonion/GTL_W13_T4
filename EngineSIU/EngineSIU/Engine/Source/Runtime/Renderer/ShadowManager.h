#pragma once

#define _TCHAR_DEFINED
#include <d3d11.h>

#include "RendererHelpers.h"
#include "Container/Array.h"
#include "Math/Matrix.h"     // FMatrix (UE 스타일)

struct FShadowDepthRHI
{
    // --- 방향성 광원 섀도우 리소스 (CSM 기반) ---
    ID3D11Texture2D* ShadowTexture = nullptr; //텍스쳐 맵
    ID3D11ShaderResourceView* ShadowSRV = nullptr; //텍스쳐맵 srv
    TArray<ID3D11DepthStencilView*> ShadowDSVs; // 디렉셔널인경우  cascade
    TArray<ID3D11ShaderResourceView*> ShadowSRVs; // imgui용 각 텍스쳐의 srv
    
    uint32 ShadowMapResolution = 1024; // 섀도우 맵 해상도 (기본값: 1024x1024)

    void Release()
    {
        if (ShadowTexture)
        {
            ShadowTexture->Release();
            ShadowTexture = nullptr;
        }
        if (ShadowSRV)
        {
            ShadowSRV->Release();
            ShadowSRV = nullptr;
        }
        for (auto& DSV : ShadowDSVs)
        {
            if (DSV)
            {
                DSV->Release();
                DSV = nullptr;
            }
        }
        for (auto& SRV : ShadowSRVs)
        {
            if (SRV)
            {
                SRV->Release();
                SRV = nullptr;
            }
        }
    }
};

class UDirectionalLightComponent;
class FDXDBufferManager;

// TextureCubeArray 기반 섀도우 맵 리소스 관리 구조체
struct FShadowCubeMapArrayRHI
{
    ID3D11Texture2D* ShadowTexture = nullptr;         // TextureCubeArray 리소스
    ID3D11ShaderResourceView* ShadowSRV = nullptr;    // 전체 배열을 위한 SRV (메인 패스 샘플링용)
    TArray<ID3D11DepthStencilView*> ShadowDSVs;       // 각 큐브맵 슬라이스(포인트 라이트)를 위한 DSV 배열

    // ImGui 디버그용: 각 포인트 라이트(outer TArray)의 각 면(inner TArray, 0~5)에 대한 SRV
    TArray<TArray<ID3D11ShaderResourceView*>> ShadowFaceSRVs;

    uint32 ShadowMapResolution = 512; // 큐브맵 각 면의 해상도

    // 리소스 해제 함수
    void Release()
    {
        // ImGui용 개별 면 SRV 해제
        for (auto& FaceSrvArray : ShadowFaceSRVs)
        {
            for (auto& Srv : FaceSrvArray)
            {
                if (Srv)
                {
                    Srv->Release();
                    Srv = nullptr;
                }
            }
            FaceSrvArray.Empty(); // 내부 배열 비우기
        }
        ShadowFaceSRVs.Empty(); // 외부 배열 비우기

        // 개별 DSV 해제
        for (auto& DSV : ShadowDSVs)
        {
            if (DSV)
            {
                DSV->Release();
                DSV = nullptr;
            }
        }
        ShadowDSVs.Empty();

        // 전체 SRV 해제
        if (ShadowSRV)
        {
            ShadowSRV->Release();
            ShadowSRV = nullptr;
        }
        // 텍스처 해제
        if (ShadowTexture)
        {
            ShadowTexture->Release();
            ShadowTexture = nullptr;
        }
    }
};

class FEditorViewportClient;
class FShadowManager
{
public:
    friend class FShadowRenderPass;

    // --- 생성자 및 소멸자 ---
    FShadowManager();
    ~FShadowManager();

    // --- Public 멤버 함수 ---

    /**
     * 섀도우 매니저를 초기화하고 필요한 D3D 리소스를 생성합니다.
     * @param InGraphics FGraphicsDevice 포인터 (Device 및 Context 포함)
     * @param InMaxSpotShadows 지원할 최대 스포트라이트 섀도우 개수
     * @param InSpotResolution 스포트라이트 섀도우 맵 해상도
     * @param InMaxPointShadows 지원할 최대 포인트 라이트 섀도우 개수 << 추가
     * @param InPointResolution 포인트 라이트 큐브맵 면 해상도 << 추가
     * @param InNumCascades 방향성 광원 CSM 캐스케이드 개수
     * @param InDirResolution 방향성 광원 섀도우 맵 해상도
     * @return 초기화 성공 여부
     */

    bool Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager,
                    uint32_t InMaxSpotShadows = 128, uint32_t InSpotResolution = 1024,
                    uint32_t InMaxPointShadows = 128,uint32_t InPointResolution = 512, uint32_t InNumCascades = 5, uint32_t InDirResolution = 4096); // NUM Cascades 바인딩 위치가 불명확합니다.


    /** 생성된 모든 D3D 리소스를 해제합니다. */
    void Release();

    /**
     * 특정 스포트라이트 섀도우 맵 렌더링 패스를 시작하기 위해 DSV와 뷰포트를 설정하고 클리어합니다.
     * @param SliceIndex 렌더링할 Texture2DArray의 슬라이스 인덱스
     */
    void BeginSpotShadowPass(uint32_t SliceIndex);

    /**
  * 특정 포인트 라이트 섀도우 맵 렌더링 패스를 시작합니다.
  * @param SliceIndex 렌더링할 포인트 라이트의 인덱스 (DSV 선택용)
  */
    void BeginPointShadowPass(uint32_t SliceIndex); // << 추가

    /**
     * 특정 방향성 광원 캐스케이드 섀도우 맵 렌더링 패스를 시작하기 위해 DSV와 뷰포트를 설정하고 클리어합니다.
     * @param CascadeIndex 렌더링할 캐스케이드 인덱스
     */
    void BeginDirectionalShadowCascadePass(uint32_t CascadeIndex);

    /**
     * 메인 렌더링 패스에서 픽셀 셰이더가 섀도우 맵을 샘플링할 수 있도록 관련 리소스를 바인딩합니다.
     * @param SpotShadowSlot 스포트라이트 섀도우 맵 SRV 슬롯
     * @param PointShadowSlot 포인트 라이트 섀도우 맵 SRV 슬롯 << 추가
     * @param DirectionalShadowSlot 방향성 광원 섀도우 맵 SRV 슬롯
     * @param SamplerCmpSlot 비교 샘플러 슬롯
     * @param SamplerPointSlot 포인트 샘플러 슬롯 (필요시)
     */
    void BindResourcesForSampling(
        uint32_t SpotShadowSlot = static_cast<uint32_t>(EShaderSRVSlot::SRV_SpotLight), // 예시 슬롯 번호 조정
        uint32_t PointShadowSlot = static_cast<uint32_t>(EShaderSRVSlot::SRV_PointLight), // << 추가
        uint32_t DirectionalShadowSlot = static_cast<uint32_t>(EShaderSRVSlot::SRV_DirectionalLight),
        uint32_t SamplerCmpSlot = 10, // 예시 샘플러 슬롯
        uint32_t SamplerPointSlot = 11 // 예시 샘플러 슬롯
        );
    
    FShadowDepthRHI* GetSpotShadowDepthRHI() const { return SpotShadowDepthRHI; }
    FShadowCubeMapArrayRHI* GetPointShadowCubeMapRHI() const { return PointShadowCubeMapRHI; } // << 추가
    FShadowDepthRHI* GetDirectionalShadowCascadeDepthRHI() const { return DirectionalShadowCascadeDepthRHI; }

    FMatrix GetCascadeViewProjMatrix(int Idx) const;
    uint32 GetNumCasCades() const { return NumCascades; }
    float GetCascadeSplitDistance(int Idx) const { return CascadeSplits[Idx]; }

    int32 GetMaxPointLightCount() const { return MaxPointLightShadows; } 
    int32 GetMaxSpotLightCount() const { return MaxSpotLightShadows; }

private:
    
    // D3D 디바이스 및 컨텍스트
    ID3D11Device* D3DDevice = nullptr;
    ID3D11DeviceContext* D3DContext = nullptr;
    FDXDBufferManager* BufferManager = nullptr;         // 상수버퍼 바인딩 위함

    // 각 라이트 타입별 섀도우 리소스 RHI
    FShadowDepthRHI* SpotShadowDepthRHI = nullptr;
    FShadowCubeMapArrayRHI* PointShadowCubeMapRHI = nullptr; // << 추가
    FShadowDepthRHI* DirectionalShadowCascadeDepthRHI = nullptr; // 방향성 광원 섀도우 맵을 위한 Depth RHI
    //uint32 MaxDirectionalLightShadows = 1;


    uint32 NumCascades = 3;                             // [캐스케이드 개수] : **여기서 초기화**
    TArray<FMatrix> CascadesViewProjMatrices;   // 캐스케이드 ViewProj 행렬
    TArray<FMatrix> CascadesInvProjMatrices;    // 캐스케이드 InvProj 행렬
    TArray<float> CascadeSplits;                  // 캐스케이드 분할 거리 (NearClip ~ FarClip)

    // 설정 값
    uint32_t MaxSpotLightShadows = 16;
    uint32_t MaxPointLightShadows = 8; // << 추가


    // 방향성 광원 뷰-프로젝션 행렬 (CSM용)
    TArray<FMatrix> DirectionalLightViewProjMatrices;

    // 공통 샘플러
    ID3D11SamplerState* ShadowSamplerCmp = nullptr;   // PCF 용
    ID3D11SamplerState* ShadowPointSampler = nullptr; // 하드 섀도우 또는 VSM/ESM의 초기 샘플링용

    // --- Private 멤버 함수 (리소스 생성/해제 헬퍼) ---
    bool CreateSpotShadowResources();
    void ReleaseSpotShadowResources();

    bool CreatePointShadowResources(); // << 추가
    void ReleasePointShadowResources(); // << 추가

    bool CreateDirectionalShadowResources();
    void ReleaseDirectionalShadowResources();

    /* 캐스케이드 분할 관련 Matrix를 갱신합니다 */
    void UpdateCascadeMatrices(const std::shared_ptr<FEditorViewportClient>& Viewport, UDirectionalLightComponent* DirectionalLight);

    /** 섀도우 샘플링에 사용될 D3D 샘플러 상태(비교 샘플러 등)를 생성합니다. */
    bool CreateSamplers();
    void ReleaseSamplers();
    
};
