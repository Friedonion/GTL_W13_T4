#pragma once
#define _TCHAR_DEFINED
#include "Define.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "Container/String.h"
#include "Container/Array.h"
#include "Container/Map.h"
#include "Engine/Texture.h"
#include "GraphicDevice.h"
#include "UserInterface/Console.h"

// ShaderStage 열거형
enum class EShaderStage
{
    Vertex,
    Pixel,
    Compute,
    Geometry,
};
struct QuadVertex
{
    float Position[3];
    float TexCoord[2];
};

class FDXDBufferManager
{
public:
    QuadVertex Q;

    FDXDBufferManager() = default;
    void Initialize(ID3D11Device* DXDevice, ID3D11DeviceContext* DXDeviceContext);

    // 템플릿을 활용한 버텍스 버퍼 생성 (정적/동적) - FString / FWString
    template<typename T>
    HRESULT CreateVertexBuffer(const FString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0);
    template<typename T>
    HRESULT CreateVertexBuffer(const FWString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0);

    template<typename T>
    HRESULT CreateIndexBuffer(const FString& KeyName, const TArray<T>& Indices, FIndexInfo& OutIndexInfo, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0);
    template<typename T>
    HRESULT CreateIndexBuffer(const FWString& KeyName, const TArray<T>& Indices, FIndexInfo& OutIndexInfo, D3D11_USAGE Usage = D3D11_USAGE_DEFAULT, UINT CpuAccessFlags = 0);

    template<typename T>
    HRESULT CreateDynamicVertexBuffer(const FString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo);

    // 템플릿 헬퍼 함수: 내부에서 버퍼 생성 로직 통합
    template<typename T>
    HRESULT CreateVertexBufferInternal(const FString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo,
        D3D11_USAGE Usage, UINT CpuAccessFlags);

    template<typename T>
    HRESULT CreateVertexBufferInternal(const FWString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo,
        D3D11_USAGE Usage, UINT CpuAccessFlags);

    HRESULT CreateUnicodeTextBuffer(const FWString& Text, FBufferInfo& OutBufferInfo, uint32 BitmapWidth, uint32 BitmapHeight, uint32 ColCount, uint32 RowCount);

    void SetStartUV(wchar_t Hangul, FVector2D& UVOffset);
    
    void ReleaseBuffers();
    void ReleaseConstantBuffer();
    void ReleaseStructuredBuffer();

    template<typename T>
    HRESULT CreateBufferGeneric(const FString& KeyName, T* Data, UINT ByteWidth, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags);

    template<typename T>
    HRESULT CreateStructuredBufferGeneric(const FString& KeyName, T* Data, int32 NumElements, D3D11_USAGE Usage, UINT CpuAccessFlags);
    
    template<typename T>
    void UpdateConstantBuffer(const FString& Key, const T& Data) const;

    template<typename T>
    void UpdateConstantBuffer(const FString& Key, const TArray<T>& Data) const;

    template<typename T>
    void UpdateStructuredBuffer(const FString& Key, const TArray<T>& Data) const;
    
    template<typename T>
    void UpdateDynamicVertexBuffer(const FString& KeyName, const TArray<T>& Vertices) const;

    void BindConstantBuffers(const TArray<FString>& Keys, UINT StartSlot, EShaderStage Stage) const;
    void BindConstantBuffer(const FString& Key, UINT StartSlot, EShaderStage Stage) const;
    void BindStructuredBufferSRV(const FString& Key, UINT StartSlot, EShaderStage Stage) const;
    
    template<typename T>
    static void SafeRelease(T*& ComObject);
    
    FVertexInfo GetVertexBuffer(const FString& InName) const;
    FIndexInfo GetIndexBuffer(const FString& InName) const;
    FVertexInfo GetTextVertexBuffer(const FWString& InName) const;
    FIndexInfo GetTextIndexBuffer(const FWString& InName) const;
    ID3D11Buffer* GetConstantBuffer(const FString& InName) const;
    ID3D11Buffer* GetStructuredBuffer(const FString& InName) const;
    ID3D11ShaderResourceView* GetStructuredBufferSRV(const FString& InName) const;

    void GetQuadBuffer(FVertexInfo& OutVertexInfo, FIndexInfo& OutIndexInfo);
    void GetTextBuffer(const FWString& Text, FVertexInfo& OutVertexInfo, FIndexInfo& OutIndexInfo);
    void CreateQuadBuffer();
    
private:
    // 16바이트 정렬
    inline UINT Align16(UINT Size) const { return (Size + 15) & ~15; }
    
private:
    ID3D11Device* DXDevice = nullptr;
    ID3D11DeviceContext* DXDeviceContext = nullptr;

    TMap<FString, FVertexInfo> VertexBufferPool;
    TMap<FString, FIndexInfo> IndexBufferPool;
    TMap<FString, ID3D11Buffer*> ConstantBufferPool;
    TMap<FString, ID3D11Buffer*> StructuredBufferPool;
    TMap<FString, ID3D11ShaderResourceView*> StructuredBufferSRVPool;
    TMap<FString, int32> StructuredBufferElementSizePool;

    TMap<FWString, FBufferInfo> TextAtlasBufferPool;
    TMap<FWString, FVertexInfo> TextAtlasVertexBufferPool;
    TMap<FWString, FIndexInfo> TextAtlasIndexBufferPool;
};

// 템플릿 함수 구현부

template<typename T>
HRESULT FDXDBufferManager::CreateVertexBufferInternal(const FString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo,
    D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    if (!KeyName.IsEmpty() && VertexBufferPool.Contains(KeyName))
    {
        OutVertexInfo = VertexBufferPool[KeyName];
        return S_OK;
    }
    uint32_t Stride = sizeof(T);
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = Usage;
    BufferDesc.ByteWidth = Stride * Vertices.Num();
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = CpuAccessFlags;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Vertices.GetData();

    ID3D11Buffer* NewBuffer = nullptr;
    const HRESULT Result = DXDevice->CreateBuffer(&BufferDesc, &InitData, &NewBuffer);
    if (FAILED(Result))
    {
        return Result;
    }

    OutVertexInfo.NumVertices = static_cast<uint32>(Vertices.Num());
    OutVertexInfo.VertexBuffer = NewBuffer;
    OutVertexInfo.Stride = Stride;
    VertexBufferPool.Add(KeyName, OutVertexInfo);

    return S_OK;
}
template<typename T>
HRESULT FDXDBufferManager::CreateIndexBuffer(const FString& KeyName, const TArray<T>& Indices, FIndexInfo& OutIndexInfo, D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    if (!KeyName.IsEmpty() && IndexBufferPool.Contains(KeyName))
    {
        OutIndexInfo = IndexBufferPool[KeyName];
        return S_OK;
    }

    D3D11_BUFFER_DESC IndexBufferDesc = {};
    IndexBufferDesc.Usage = Usage;
    IndexBufferDesc.ByteWidth = Indices.Num() * sizeof(uint32);
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.CPUAccessFlags = CpuAccessFlags;

    D3D11_SUBRESOURCE_DATA IndexInitData = {};
    IndexInitData.pSysMem = Indices.GetData();

    ID3D11Buffer* NewBuffer = nullptr;
    const HRESULT Result = DXDevice->CreateBuffer(&IndexBufferDesc, &IndexInitData, &NewBuffer);
    if (FAILED(Result))
    {
        return Result;
    }

    OutIndexInfo.NumIndices = static_cast<uint32>(Indices.Num());
    OutIndexInfo.IndexBuffer = NewBuffer;
    IndexBufferPool.Add(KeyName, FIndexInfo{static_cast<uint32>(Indices.Num()), NewBuffer});


    return S_OK;
}

template<typename T>
HRESULT FDXDBufferManager::CreateVertexBuffer(const FString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo, D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    return CreateVertexBufferInternal(KeyName, Vertices, OutVertexInfo, Usage, CpuAccessFlags);
}


// FWString 전용 버텍스 버퍼 생성 (내부)
template<typename T>
HRESULT FDXDBufferManager::CreateVertexBufferInternal(const FWString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo,
    D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    if (!KeyName.empty() && TextAtlasVertexBufferPool.Contains(KeyName))
    {
        OutVertexInfo = TextAtlasVertexBufferPool[KeyName];
        return S_OK;
    }
    uint32_t Stride = sizeof(T);
    D3D11_BUFFER_DESC BufferDesc = {};
    BufferDesc.Usage = Usage;
    BufferDesc.ByteWidth = Stride * Vertices.Num();
    BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    BufferDesc.CPUAccessFlags = CpuAccessFlags;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Vertices.GetData();

    ID3D11Buffer* NewBuffer = nullptr;
    const HRESULT Result = DXDevice->CreateBuffer(&BufferDesc, &InitData, &NewBuffer);
    if (FAILED(Result))
    {
        return Result;
    }

    OutVertexInfo.NumVertices = static_cast<uint32>(Vertices.Num());
    OutVertexInfo.VertexBuffer = NewBuffer;
    OutVertexInfo.Stride = Stride;
    TextAtlasVertexBufferPool.Add(KeyName, OutVertexInfo);

    return S_OK;
}

// FWString 전용 인덱스 버퍼 생성
template<typename T>
HRESULT FDXDBufferManager::CreateIndexBuffer(const FWString& KeyName, const TArray<T>& Indices, FIndexInfo& OutIndexInfo, D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    if (!KeyName.empty() && TextAtlasIndexBufferPool.Contains(KeyName))
    {
        OutIndexInfo = TextAtlasIndexBufferPool[KeyName];
        return S_OK;
    }

    D3D11_BUFFER_DESC IndexBufferDesc = {};
    IndexBufferDesc.Usage = Usage;
    IndexBufferDesc.ByteWidth = Indices.Num() * sizeof(uint32);
    IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IndexBufferDesc.CPUAccessFlags = CpuAccessFlags;

    D3D11_SUBRESOURCE_DATA IndexInitData = {};
    IndexInitData.pSysMem = Indices.GetData();

    ID3D11Buffer* NewBuffer = nullptr;
    const HRESULT Result = DXDevice->CreateBuffer(&IndexBufferDesc, &IndexInitData, &NewBuffer);
    if (FAILED(Result))
    {
        return Result;
    }

    OutIndexInfo.NumIndices = static_cast<uint32>(Indices.Num());
    OutIndexInfo.IndexBuffer = NewBuffer;
    TextAtlasIndexBufferPool.Add(KeyName, FIndexInfo{static_cast<uint32>(Indices.Num()), NewBuffer});

    return S_OK;
}

template<typename T>
HRESULT FDXDBufferManager::CreateVertexBuffer(const FWString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo, D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    return CreateVertexBufferInternal(KeyName, Vertices, OutVertexInfo, Usage, CpuAccessFlags);
}


template<typename T>
HRESULT FDXDBufferManager::CreateDynamicVertexBuffer(const FString& KeyName, const TArray<T>& Vertices, FVertexInfo& OutVertexInfo)
{
    return CreateVertexBufferInternal(KeyName, Vertices, OutVertexInfo, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
}

template<typename T>
HRESULT FDXDBufferManager::CreateBufferGeneric(const FString& KeyName, T* Data, UINT ByteWidth, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    if (ConstantBufferPool.Contains(KeyName))
    {
        return S_OK;
    }
    
    ByteWidth = Align16(ByteWidth);

    D3D11_BUFFER_DESC Desc = {};
    Desc.ByteWidth = ByteWidth;
    Desc.Usage = Usage;
    Desc.BindFlags = BindFlags;
    Desc.CPUAccessFlags = CpuAccessFlags;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Data;

    ID3D11Buffer* Buffer = nullptr;
    const HRESULT Result = DXDevice->CreateBuffer(&Desc, Data ? &InitData : nullptr, &Buffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Error Create Constant Buffer!"));
        return Result;
    }

    ConstantBufferPool.Add(KeyName, Buffer);
    return S_OK;
}

template <typename T>
HRESULT FDXDBufferManager::CreateStructuredBufferGeneric(const FString& KeyName, T* Data, int32 NumElements, D3D11_USAGE Usage, UINT CpuAccessFlags)
{
    if (StructuredBufferPool.Contains(KeyName))
    {
        return S_OK;
    }
    
    D3D11_BUFFER_DESC Desc = {};
    Desc.ByteWidth = Align16(sizeof(T) * NumElements);
    Desc.Usage = Usage;
    Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    Desc.CPUAccessFlags = CpuAccessFlags;
    Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    Desc.StructureByteStride = sizeof(T);

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = Data;

    ID3D11Buffer* Buffer = nullptr;
    HRESULT Result = DXDevice->CreateBuffer(&Desc, Data ? &InitData : nullptr, &Buffer);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Error Create Structured Buffer!"));
        return Result;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SrvDesc.Buffer.FirstElement = 0;
    SrvDesc.Buffer.NumElements = NumElements;

    ID3D11ShaderResourceView* SRV = nullptr;
    Result = DXDevice->CreateShaderResourceView(Buffer, nullptr, &SRV);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Error Create Structured Buffer SRV!"));
        return Result;
    }

    StructuredBufferPool.Add(KeyName, Buffer);
    StructuredBufferSRVPool.Add(KeyName, SRV);
    StructuredBufferElementSizePool.Add(KeyName, NumElements);
    
    return S_OK;
}

template<typename T>
void FDXDBufferManager::UpdateConstantBuffer(const FString& Key, const T& Data) const
{
    ID3D11Buffer* Buffer = GetConstantBuffer(Key);
    if (!Buffer)
    {
        UE_LOG(ELogLevel::Error, TEXT("UpdateConstantBuffer 호출: 키 %s에 해당하는 buffer가 없습니다."), *Key);
        return;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    const HRESULT Result = DXDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Buffer Map 실패, HRESULT: 0x%X"), Result);
        return;
    }
    memcpy(MappedResource.pData, &Data, sizeof(T));
    DXDeviceContext->Unmap(Buffer, 0);
}

template<typename T>
void FDXDBufferManager::UpdateConstantBuffer(const FString& Key, const TArray<T>& Data) const
{
    ID3D11Buffer* Buffer = GetConstantBuffer(Key);
    if (!Buffer)
    {
        UE_LOG(ELogLevel::Error, TEXT("UpdateConstantBuffer 호출: 키 %s에 해당하는 buffer가 없습니다."), *Key);
        return;
    }

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    const HRESULT Result = DXDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Buffer Map 실패, HRESULT: 0x%X"), Result);
        return;
    }
    memcpy(MappedResource.pData, Data.GetData(), sizeof(T) * Data.Num());
    DXDeviceContext->Unmap(Buffer, 0);
}

template <typename T>
void FDXDBufferManager::UpdateStructuredBuffer(const FString& Key, const TArray<T>& Data) const
{
    ID3D11Buffer* Buffer = GetStructuredBuffer(Key);
    if (!Buffer)
    {
        UE_LOG(ELogLevel::Error, TEXT("UpdateConstantBuffer 호출: 키 %s에 해당하는 buffer가 없습니다."), *Key);
        return;
    }

    // 버퍼 생성할때 지정한 NumElements 만큼 최대 크기 제한
    int32 ElementSize = StructuredBufferElementSizePool[Key];
    ElementSize = FMath::Max(0, FMath::Min(ElementSize, Data.Num()));

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    const HRESULT Result = DXDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("Buffer Map 실패, HRESULT: 0x%X"), Result);
        return;
    }
    memcpy(MappedResource.pData, Data.GetData(), sizeof(T) * ElementSize);
    DXDeviceContext->Unmap(Buffer, 0);
}

template<typename T>
void FDXDBufferManager::UpdateDynamicVertexBuffer(const FString& KeyName, const TArray<T>& Vertices) const
{
    if (!VertexBufferPool.Contains(KeyName))
    {
        UE_LOG(ELogLevel::Error, TEXT("UpdateDynamicVertexBuffer 호출: 키 %s에 해당하는 버텍스 버퍼가 없습니다."), *KeyName);
        return;
    }
    const FVertexInfo VbInfo = VertexBufferPool[KeyName];

    D3D11_MAPPED_SUBRESOURCE Mapped;
    const HRESULT Result = DXDeviceContext->Map(VbInfo.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    if (FAILED(Result))
    {
        UE_LOG(ELogLevel::Error, TEXT("VertexBuffer Map 실패, HRESULT: 0x%X"), Result);
        return;
    }

    memcpy(Mapped.pData, Vertices.GetData(), sizeof(T) * Vertices.Num());
    DXDeviceContext->Unmap(VbInfo.VertexBuffer, 0);
}

template<typename T>
void FDXDBufferManager::SafeRelease(T*& ComObject)
{
    if (ComObject)
    {
        ComObject->Release();
        ComObject = nullptr;
    }
}

