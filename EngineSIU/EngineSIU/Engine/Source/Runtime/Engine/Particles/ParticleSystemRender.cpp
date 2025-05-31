#include "ParticleHelper.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleModuleRequired.h"
#include "Math/Matrix.h"

FDynamicSpriteEmitterReplayDataBase::FDynamicSpriteEmitterReplayDataBase()
: MaterialInterface(nullptr)
, RequiredModule(nullptr)
, NormalsSphereCenter(FVector::ZeroVector)
, NormalsCylinderDirection(FVector::ZeroVector)
, InvDeltaSeconds(0.0f)
, MaxDrawCount(0)
, OrbitModuleOffset(0)
, DynamicParameterDataOffset(0)
, LightDataOffset(0)
, LightVolumetricScatteringIntensity(0)
, CameraPayloadOffset(0)
, SubUVDataOffset(0)
, SubImages_Horizontal(1)
, SubImages_Vertical(1)
, bUseLocalSpace(false)
, bLockAxis(false)
, ScreenAlignment(0)
, LockAxisFlag(0)
, EmitterRenderMode(0)
, EmitterNormalsMode(0)
, PivotOffset(-0.5f, -0.5f)
, bUseVelocityForMotionBlur(false)
, bRemoveHMDRoll(false)
, MinFacingCameraBlendDistance(0.f)
, MaxFacingCameraBlendDistance(0.f)
{
}

FDynamicSpriteEmitterReplayDataBase::~FDynamicSpriteEmitterReplayDataBase()
{
    delete RequiredModule;
}

void FDynamicSpriteEmitterReplayDataBase::Serialize(FArchive& Ar)
{
    // Call parent implementation
    FDynamicEmitterReplayDataBase::Serialize( Ar );

    Ar << ScreenAlignment;
    Ar << bUseLocalSpace;
    Ar << bLockAxis;
    Ar << LockAxisFlag;
    Ar << MaxDrawCount;

    int32 EmitterRenderModeInt = EmitterRenderMode;
    Ar << EmitterRenderModeInt;
    EmitterRenderMode = EmitterRenderModeInt;

    Ar << OrbitModuleOffset;
    Ar << DynamicParameterDataOffset;
    Ar << LightDataOffset;
    Ar << LightVolumetricScatteringIntensity;
    Ar << CameraPayloadOffset;

    Ar << EmitterNormalsMode;
    Ar << NormalsSphereCenter;
    Ar << NormalsCylinderDirection;

    // Ar << MaterialInterface;

    Ar << PivotOffset;

    Ar << bUseVelocityForMotionBlur;
    Ar << bRemoveHMDRoll;
    Ar << MinFacingCameraBlendDistance;
    Ar << MaxFacingCameraBlendDistance;
}

void FDynamicSpriteEmitterDataBase::SortSpriteParticles(int32 SortMode, bool bLocalSpace, int32 ParticleCount, const uint8* ParticleData, int32 ParticleStride, const uint16* ParticleIndices, const FMatrix& LocalToWorld) const
{
    if (SortMode == static_cast<int32>(EParticleSortMode::PSORTMODE_ViewProjDepth))
	{

	}
	else if (SortMode == static_cast<int32>(EParticleSortMode::PSORTMODE_DistanceToView))
	{

	}
	else if (SortMode == static_cast<int32>(EParticleSortMode::PSORTMODE_Age_OldestFirst))
	{

	}
	else if (SortMode == static_cast<int32>(EParticleSortMode::PSORTMODE_Age_NewestFirst))
	{

	}
}

FVector2D GetParticleSize(const FBaseParticle& Particle, const FDynamicSpriteEmitterReplayDataBase& Source)
{
    FVector2D Size;
    Size.X = FMath::Abs(Particle.Size.X * Source.Scale.X);
    Size.Y = FMath::Abs(Particle.Size.Y * Source.Scale.Y);
    if (Source.ScreenAlignment == static_cast<uint8>(EParticleScreenAlignment::PSA_Square) ||
        Source.ScreenAlignment == static_cast<uint8>(EParticleScreenAlignment::PSA_FacingCameraPosition) ||
        Source.ScreenAlignment == static_cast<uint8>(EParticleScreenAlignment::PSA_FacingCameraDistanceBlend))
    {
        Size.Y = Size.X;
    }

    return Size;
}

void FDynamicSpriteEmitterData::Init(bool bInSelected)
{
    bSelected = bInSelected;

    bUsesDynamicParameter = GetSourceData()->DynamicParameterDataOffset > 0;

    // We won't need this on the render thread
    //Source.MaterialInterface = nullptr;
}

bool FDynamicSpriteEmitterData::GetVertexAndIndexData(void* VertexData, void* FillIndexData, const FVector& InCameraPosition, const FMatrix& InLocalToWorld, uint32 InstanceFactor) const
{
    int32 ParticleCount = Source.ActiveParticleCount;
    if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
    {
        ParticleCount = Source.MaxDrawCount;
    }

    // Put the camera origin in the appropriate coordinate space.
    FVector CameraPosition = InCameraPosition;
    if (Source.bUseLocalSpace)
    {
        FMatrix InvSelf = FMatrix::Inverse(InLocalToWorld);
        CameraPosition = InvSelf.TransformPosition(InCameraPosition);
    }

    // Pack the data
    int32 ParticleIndex = 0;

    int32 VertexStride = sizeof(FParticleSpriteVertex);
    uint8* TempVert = (uint8*)VertexData;

    FParticleSpriteVertex* FillVertex;

    int32 SubImageIndex = 0;

    const uint8* ParticleData = Source.DataContainer.ParticleData;
    const uint16* ParticleIndices = Source.DataContainer.ParticleIndices;
    const int32 OrderedIndices = 0; // Todo: Need to change the real index.

    for (int32 i = 0; i < ParticleCount; i++)
    {
        ParticleIndex = OrderedIndices ? 0 : i; // Todo: Need to change the real index.
        DECLARE_PARTICLE_CONST(Particle, ParticleData + Source.ParticleStride * ParticleIndices[ParticleIndex])

        // Prefetch
        if (i + 1 < ParticleCount)
        {
            int32 NextIndex = OrderedIndices ? /*OrderedIndices[*/ i + 1 /*]*/ : (i + 1);
            DECLARE_PARTICLE_CONST(NextParticle, ParticleData + Source.ParticleStride * ParticleIndices[NextIndex])
            // Prefetch(&NextParticle);
        }

        const FVector2D Size = GetParticleSize(Particle, Source);

        FVector ParticlePosition = Particle.Location;
        FVector ParticleOldPosition = Particle.OldLocation;
        
        if (Source.CameraPayloadOffset != 0)
        {
            
        }

        if (Source.SubUVDataOffset > 0)
        {
            
        }

        for (uint32 Factor = 0; Factor < InstanceFactor; Factor++)
        {
            FillVertex = reinterpret_cast<FParticleSpriteVertex*>(TempVert);
            FillVertex->Position = FVector(ParticlePosition);
            FillVertex->RelativeTime = Particle.RelativeTime;
            FillVertex->OldPosition = FVector(ParticleOldPosition);
            
            // Create a floating point particle ID from the counter, map into approximately 0-1
            FillVertex->ParticleId = (Particle.Flags & (~0xFE000000)) / 10000.0f;
            FillVertex->Size = FVector2D(GetParticleSizeWithUVFlipInSign(Particle, Size));
            FillVertex->Rotation = Particle.Rotation;
            FillVertex->SubImageIndex = SubImageIndex;
            FillVertex->Color = Particle.Color;

            TempVert += VertexStride;
        }
    }
    
    return true;
}

void FDynamicSpriteEmitterData::GetDynamicMeshElementsEmitter() const
{
    
}

FDynamicMeshEmitterData::FDynamicMeshEmitterData(const UParticleModuleRequired* RequiredModule)
    : FDynamicSpriteEmitterDataBase(RequiredModule)
    //, LastFramePreRendered(-1)
    , StaticMesh( nullptr )
    //, MeshTypeDataOffset(0xFFFFFFFF)
    //, bApplyPreRotation(false)
    //, bUseMeshLockedAxis(false)
    //, bUseCameraFacing(false)
    //, bApplyParticleRotationAsSpin(false)
    //, bFaceCameraDirectionRatherThanPosition(false)
    //, CameraFacingOption(0)
    //, LastCalculatedMeshLOD(0)
    , EmitterInstance(nullptr)
{
    
}
