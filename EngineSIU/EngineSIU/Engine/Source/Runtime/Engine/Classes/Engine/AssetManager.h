#pragma once
#include "StaticMesh.h"
#include "DirectXTK/Keyboard.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UParticleSystem;
class UAnimationAsset;
class USkeleton;
class USkeletalMesh;

enum class EAssetType : uint8
{
    None,
    StaticMesh,
    SkeletalMesh,
    Skeleton,
    Animation,
    Texture2D,
    Material,
    ParticleSystem,
    PhysicsAsset,
    MAX
};

struct FAssetInfo
{
    FName AssetName;        // Asset의 이름
    FName PackagePath;      // Asset의 패키지 경로
    FString SourceFilePath; // 원본 파일 경로
    EAssetType AssetType = EAssetType::None;   // Asset의 타입
    uint32 Size;            // Asset의 크기 (바이트 단위)
    UObject* AssetObject = nullptr;

    [[nodiscard]] FString GetFullPath() const { return PackagePath.ToString() / AssetName.ToString(); }

    void Serialize(FArchive& Ar)
    {
        int8 Type = static_cast<int8>(AssetType);

        Ar << AssetName
           << PackagePath
           << SourceFilePath
           << Type
           << Size;

        AssetType = static_cast<EAssetType>(Type);
    }
};

struct FAssetRegistry
{
    TMap<FName, FAssetInfo> PathNameToAssetInfo;
};

struct FAssetLoadResult
{
    TArray<USkeleton*> Skeletons;
    TArray<USkeletalMesh*> SkeletalMeshes;
    TArray<UStaticMesh*> StaticMeshes;
    TArray<UMaterial*> Materials;
    TArray<UAnimationAsset*> Animations;
};

class UAssetManager : public UObject
{
    DECLARE_CLASS(UAssetManager, UObject)

private:
    std::unique_ptr<FAssetRegistry> AssetRegistry;

public:
    UAssetManager() = default;
    virtual ~UAssetManager() override = default;

    static bool IsInitialized();

    /** UAssetManager를 가져옵니다. */
    static UAssetManager& Get();

    /** UAssetManager가 존재하면 가져오고, 없으면 nullptr를 반환합니다. */
    static UAssetManager* GetIfInitialized();
    
    void InitAssetManager();

    void ReleaseAssetManager();

    const TMap<FName, FAssetInfo>& GetAssetRegistry();
    TMap<FName, FAssetInfo>& GetAssetRegistryRef();

    UObject* GetAsset(EAssetType AssetType, const FName& Name) const;
    USkeletalMesh* GetSkeletalMesh(const FName& Name) const;
    UStaticMesh* GetStaticMesh(const FName& Name) const;
    USkeleton* GetSkeleton(const FName& Name) const;
    UMaterial* GetMaterial(const FName& Name) const;
    UAnimationAsset* GetAnimation(const FName& Name) const;
    UParticleSystem* GetParticleSystem(const FName& Name) const;

    void GetAssetKeys(EAssetType AssetType, TSet<FName>& OutKeys) const;
    void GetAssetKeys(EAssetType AssetType, TArray<FName>& OutKeys) const;

    const FName GetAssetKeyByObject(EAssetType AssetType, const UObject* AssetObject) const;
    
    void AddAssetInfo(const FAssetInfo& Info);
    
    void AddAsset(const FName& Key, UObject* AssetObject);

    bool SavePhysicsAsset(const FString& FilePath, UPhysicsAsset* PhysicsAsset);

    bool SaveParticleSystemAsset(const FString& FilePath, UParticleSystem* ParticleSystemAsset);

private:
    inline static TMap<EAssetType, TMap<FName, UObject*>> AssetMap;
    
    double FbxLoadTime = 0.0;
    double BinaryLoadTime = 0.0;

    EAssetType GetAssetType(const UObject* AssetObject) const;

    void LoadContentFiles();

    void HandleFBX(const FAssetInfo& AssetInfo);

    void AddToAssetMap(const FAssetLoadResult& Result, const FString& BaseName, const FAssetInfo& BaseAssetInfo);

    bool LoadFbxBinary(const FString& FilePath, FAssetLoadResult& Result, const FString& BaseName, const FString& FolderPath);

    bool SaveFbxBinary(const FString& FilePath, FAssetLoadResult& Result, const FString& BaseName, const FString& FolderPath);

    void HandlePhysicsAsset(FAssetInfo& AssetInfo);

    void HandleParticleSystemAsset(FAssetInfo& AssetInfo);
    
    static constexpr uint32 Version = 1;

    bool SerializeVersion(FArchive& Ar);

    bool SerializeAssetLoadResult(FArchive& Ar, FAssetLoadResult& Result, const FString& BaseName, const FString& FolderPath);
};
