#pragma once
#include "Engine/EditorEngine.h"
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"

class USkeletalMesh;
struct FReferenceSkeleton;
class USkeletalMeshComponent;
class UAnimDataModel;
class SkeletalMeshViewerPanel : public UEditorPanel
{
public:
    SkeletalMeshViewerPanel();

    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

    void SetSkeletalMesh(USkeletalMesh* SMesh);

    int32 GetSelectedBoneIndex() const;
    FString GetSelectedBoneName() const;

    void ClearRefSkeletalMeshComponent();
private:
    float Width = 0, Height = 0;
    USkeletalMesh* SkeletalMesh;

    void LoadBoneIcon();
    void CopyRefSkeleton();

    void RenderBoneTree(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, UEditorEngine* Engine);

    void RenderAnimationSequence(const FReferenceSkeleton& RefSkeleton, UEditorEngine* Engine); // 
    void RenderAnimationPanel(float PanelPosX, float PanelTopY, float PanelWidth, float PanelHeight);
    FString GetCleanBoneName(const FString& InFullName);

    ID3D11ShaderResourceView* BoneIconSRV = nullptr;
    ID3D11ShaderResourceView* NonWeightBoneIconSRV = nullptr;

    int32 SelectedBoneIndex = INDEX_NONE;

    FReferenceSkeleton* CopiedRefSkeleton = nullptr;
    USkeletalMeshComponent* RefSkeletalMeshComponent = nullptr;

    UAnimDataModel* PrevAnimDataModel = nullptr;
    
    int32 PreviousFrame = 0;
    int32 SelectedTrackIndex_ForRename = INDEX_NONE;
    int32 SelectedNotifyGlobalIndex_ForRename = INDEX_NONE;
    TCHAR RenameTrackBuffer[256];
    TCHAR RenameNotifyBuffer[256];
    
private:
    char NewNotifyNameBuffer[128] = "NewNotify";
    float NewNotifyTime = 0.0f;
    float RenameNotifyDuration = 1.0f; 

};
