#pragma once
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"

class USkeletalMesh;
struct FReferenceSkeleton;
class USkeletalMeshComponent;
class UBodySetup;

class PhysicsAssetViewerPanel : public UEditorPanel
{
public:
    PhysicsAssetViewerPanel();

    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

    void SetSkeletalMesh(USkeletalMesh * SMesh);

    int32 GetSelectedBoneIndex() const;
    FString GetSelectedBoneName() const;

    void ClearRefSkeletalMeshComponent();

    void AddBody(int32 BoneIndex, const FName& BoneName);
    void RemoveBody(const FName& BoneName);

    void AddConstraint(const UBodySetup* Body1, const UBodySetup* Body2);
    void RemoveConstraint(int32 ConstraintIndex);

private:
    float Width = 0;
    float Height = 0;
    USkeletalMesh* SkeletalMesh;

    void LoadBoneIcon();
    void CopyRefSkeleton();

    void RenderBoneTree(const FReferenceSkeleton & RefSkeleton, int32 BoneIndex, UEditorEngine * Engine);

    FString GetCleanBoneName(const FString & InFullName);

    ID3D11ShaderResourceView* BoneIconSRV = nullptr;
    ID3D11ShaderResourceView* NonWeightBoneIconSRV = nullptr;
    ID3D11ShaderResourceView* BodyInstanceIconSRV = nullptr;

    int32 SelectedBoneIndex = INDEX_NONE;
    int32 SelectedBodyIndex = INDEX_NONE;
    int32 SelectedConstraintIndex = INDEX_NONE;

    FReferenceSkeleton* CopiedRefSkeleton = nullptr;
    USkeletalMeshComponent* RefSkeletalMeshComponent = nullptr;


};
