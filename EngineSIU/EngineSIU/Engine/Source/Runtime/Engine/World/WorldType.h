#pragma once
#include "Engine/Source/Runtime/Core/HAL/PlatformType.h"

enum class EWorldType
{
    None,
    Game,
    Editor,
    PIE,
    EditorPreview,
    GamePreview,
    GameRPC,
    SkeletalViewer,
    ParticleViewer,
    PhysicsAssetViewer,
    Inactive
};

// TODO : EWolrldType과 EWorldTypeBitFlag 혼용 중. EWorldTypeBitFlag으로 통일하기 
enum class EWorldTypeBitFlag
{
    None = 0,
    Game = 1 << 0,
    Editor = 1 << 1,
    PIE = 1 << 2,
    EditorPreview = 1 << 3,
    GamePreview = 1 << 4,
    GameRPC = 1 << 5,
    SkeletalViewer = 1 << 6,
    ParticleViewer = 1 << 7,
    PhysicsAssetViewer = 1 << 8,  
    Inactive = 1 << 9   
};

inline EWorldTypeBitFlag operator|(EWorldTypeBitFlag A, EWorldTypeBitFlag B)
{
    return static_cast<EWorldTypeBitFlag>(
        static_cast<uint32>(A) | static_cast<uint32>(B)
    );
}

inline EWorldTypeBitFlag operator&(EWorldTypeBitFlag A, EWorldTypeBitFlag B)
{
    return static_cast<EWorldTypeBitFlag>(
        static_cast<uint32>(A) & static_cast<uint32>(B)
    );
}

inline bool HasFlag(EWorldTypeBitFlag Mask, EWorldTypeBitFlag Flag)
{
    return (static_cast<uint32>(Mask) & static_cast<uint32>(Flag)) != 0;
}
