#include "SkeletalMeshViewerPanel.h"
#include "Engine/EditorEngine.h"
#include <ReferenceSkeleton.h>

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Engine/Classes/Engine/SkeletalMesh.h"
#include "Engine/Classes/Animation/Skeleton.h"
#include "Engine/Classes/Engine/FbxLoader.h"
#include "ThirdParty/ImGui/include/ImGui/imgui_neo_sequencer.h"
#include "Engine/Classes/Components/SkeletalMeshComponent.h"
#include "Engine/Classes/Animation/AnimTypes.h"
#include "UnrealEd/ImGuiWidget.h"
#include "Animation/AnimSoundNotify.h"
#include "SoundManager.h"

SkeletalMeshViewerPanel::SkeletalMeshViewerPanel()
{
    SetSupportedWorldTypes(EWorldTypeBitFlag::SkeletalViewer);
}

void SkeletalMeshViewerPanel::Render()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }

    if (BoneIconSRV == nullptr || NonWeightBoneIconSRV == nullptr) {
        LoadBoneIcon();
    }

    /* Pre Setup */
    float PanelWidth = (Width) * 0.2f - 6.0f;
    float PanelHeight = (Height) * 0.7f;

    float PanelPosX = (Width) * 0.8f+5.0f;
    float PanelPosY = 5.0f;

    ImVec2 MinSize(140, 100);
    ImVec2 MaxSize(FLT_MAX, 1000);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);
    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    constexpr ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    
    
    if (Engine->ActiveWorld) {
        if (Engine->ActiveWorld->WorldType == EWorldType::SkeletalViewer) {

            if (CopiedRefSkeleton == nullptr) {
                CopyRefSkeleton(); // 선택된 액터/컴포넌트로부터 스켈레톤 정보 복사
            }

            // CopiedRefSkeleton이 여전히 null이면 렌더링하지 않음
            if (CopiedRefSkeleton == nullptr || CopiedRefSkeleton->RawRefBoneInfo.IsEmpty()) {
                ImGui::Begin("Bone Hierarchy", nullptr, PanelFlags); // 창은 표시하되 내용은 비움
                ImGui::Text("No skeleton selected or skeleton has no bones.");
                ImGui::End();
                return;
            }

            ImGui::Begin("Bone Hierarchy", nullptr, PanelFlags); // 창 이름 변경

            // 검색 필터 추가 (선택 사항)
            // static char BoneSearchText[128] = "";
            // ImGui::InputText("Search", BoneSearchText, IM_ARRAYSIZE(BoneSearchText));
            // FString SearchFilter(BoneSearchText);

            // 루트 본부터 시작하여 트리 렌더링
            for (int32 i = 0; i < CopiedRefSkeleton->RawRefBoneInfo.Num(); ++i)
            {
                if (CopiedRefSkeleton->RawRefBoneInfo[i].ParentIndex == INDEX_NONE) // 루트 본인 경우
                {
                    // RenderBoneTree 호출 시 Engine 포인터 전달
                    RenderBoneTree(*CopiedRefSkeleton, i, Engine /*, SearchFilter */);
                }
            }
            ImGui::End();
        }
        
        RenderAnimationPanel(PanelPosX, PanelPosY, PanelWidth, PanelHeight);
        
        if (CopiedRefSkeleton) {
            RenderAnimationSequence(*CopiedRefSkeleton, Engine);
        }
        
        float ExitPanelWidth = (Width) * 0.2f - 6.0f;
        float ExitPanelHeight = 30.0f;

        const float margin = 10.0f;

        float ExitPanelPosX = Width - ExitPanelWidth;
        float ExitPanelPosY = Height - ExitPanelHeight - 10;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        ImGui::SetNextWindowSize(ImVec2(ExitPanelWidth, ExitPanelHeight), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(ExitPanelPosX, ExitPanelPosY), ImGuiCond_Always);
        
        constexpr ImGuiWindowFlags ExitPanelFlags =
            ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoBackground
            | ImGuiWindowFlags_NoScrollbar;

        ImGui::Begin("Exit Viewer", nullptr, ExitPanelFlags);
        if (ImGui::Button("Exit Viewer", ImVec2(ExitPanelWidth, ExitPanelHeight))) {
            ClearRefSkeletalMeshComponent();
            UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
            EdEngine->EndSkeletalMeshViewer();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}

void SkeletalMeshViewerPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void SkeletalMeshViewerPanel::SetSkeletalMesh(USkeletalMesh* SMesh)
{
    SkeletalMesh = SMesh;
}

int32 SkeletalMeshViewerPanel::GetSelectedBoneIndex() const
{
    return SelectedBoneIndex;
}

FString SkeletalMeshViewerPanel::GetSelectedBoneName() const
{
    if (SelectedBoneIndex == INDEX_NONE || !SkeletalMesh)
        return TEXT("");
    const auto& RefSkel = SkeletalMesh->GetSkeleton()->GetReferenceSkeleton();
    return RefSkel.RawRefBoneInfo[SelectedBoneIndex].Name.ToString();
}

void SkeletalMeshViewerPanel::ClearRefSkeletalMeshComponent()
{
    if (RefSkeletalMeshComponent)
    {
        RefSkeletalMeshComponent = nullptr;
    }
    if (CopiedRefSkeleton)
    {
        CopiedRefSkeleton = nullptr;
    }
    if (PrevAnimDataModel)
    {
        PrevAnimDataModel = nullptr;
    }
}

void SkeletalMeshViewerPanel::LoadBoneIcon()
{
    BoneIconSRV = FEngineLoop::ResourceManager.GetTexture(L"Assets/Viewer/Bone_16x.PNG")->TextureSRV;
    NonWeightBoneIconSRV = FEngineLoop::ResourceManager.GetTexture(L"Assets/Viewer/BoneNonWeighted_16x.PNG")->TextureSRV;

}

void SkeletalMeshViewerPanel::CopyRefSkeleton()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    const FReferenceSkeleton& OrigRef = Engine->SkeletalMeshViewerWorld
        ->GetSkeletalMeshComponent()->GetSkeletalMeshAsset()
        ->GetSkeleton()->GetReferenceSkeleton();

    CopiedRefSkeleton = new FReferenceSkeleton();
    CopiedRefSkeleton->RawRefBoneInfo = OrigRef.RawRefBoneInfo;
    CopiedRefSkeleton->RawRefBonePose = OrigRef.RawRefBonePose;
    CopiedRefSkeleton->InverseBindPoseMatrices = OrigRef.InverseBindPoseMatrices;
    CopiedRefSkeleton->RawNameToIndexMap = OrigRef.RawNameToIndexMap;

    RefSkeletalMeshComponent = Engine->SkeletalMeshViewerWorld->GetSkeletalMeshComponent();
}

void SkeletalMeshViewerPanel::RenderBoneTree(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, UEditorEngine* Engine /*, const FString& SearchFilter */)
{
    const FMeshBoneInfo& BoneInfo = CopiedRefSkeleton->RawRefBoneInfo[BoneIndex];
    const FString& ShortBoneName = GetCleanBoneName(BoneInfo.Name.ToString());

    // 검색 필터 적용 (선택 사항)
    // if (!SearchFilter.IsEmpty() && !ShortBoneName.Contains(SearchFilter))
    // {
    //    // 자식도 검색해야 하므로, 현재 노드가 필터에 맞지 않아도 자식은 재귀 호출
    //    bool bChildMatchesFilter = false;
    //    for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
    //    {
    //        if (RefSkeleton.RawRefBoneInfo[i].ParentIndex == BoneIndex)
    //        {
    //            // 자식 중 하나라도 필터에 맞으면 현재 노드도 표시해야 할 수 있음 (복잡해짐)
    //            // 간단하게는 현재 노드가 안 맞으면 그냥 건너뛰도록 할 수 있음
    //        }
    //    }
    //    // 간단한 필터링: 현재 노드가 안 맞으면 그냥 숨김 (자식도 안 나옴)
    //    // if (!ShortBoneName.Contains(SearchFilter)) return;
    // }

    // 1) ImGui ID 충돌 방지
    ImGui::PushID(BoneIndex);

    ImGui::Image((ImTextureID)BoneIconSRV, ImVec2(16, 16));  // 16×16 픽셀 크기
    ImGui::SameLine();

    ImGuiTreeNodeFlags NodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;
    // if (Engine->SkeletalMeshViewerWorld->SelectBoneIndex == BoneIndex) // 가상의 함수 호출
    // {
    //     NodeFlags |= ImGuiTreeNodeFlags_Selected; // 선택된 경우 Selected 플래그 추가
    // }

    // 자식이 없는 본은 리프 노드로 처리 (화살표 없음)
    bool bHasChildren = false;
    for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
    {
        if (RefSkeleton.RawRefBoneInfo[i].ParentIndex == BoneIndex)
        {
            bHasChildren = true;
            break;
        }
    }
    if (!bHasChildren)
    {
        NodeFlags |= ImGuiTreeNodeFlags_Leaf; // 자식 없으면 리프 노드
        NodeFlags &= ~ImGuiTreeNodeFlags_OpenOnArrow; // 리프 노드는 화살표로 열 필요 없음
    }

    // ImGui::TreeNodeEx (본 이름, 플래그)
    // 이름 부분만 클릭 가능하도록 하려면 ImGui::Selectable을 함께 사용하거나 커스텀 로직 필요
    // 여기서는 TreeNodeEx 자체의 클릭 이벤트를 사용
    bool bNodeOpen = ImGui::TreeNodeEx(*ShortBoneName, NodeFlags);

    // --- 클릭 이벤트 처리 ---
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) // 왼쪽 마우스 버튼 클릭 시
    {
        // 엔진에 선택된 본 인덱스 설정 (가상의 함수 호출)
        Engine->SkeletalMeshViewerWorld->SelectBoneIndex = (BoneIndex);
    }

    if (bNodeOpen) // 노드가 열려있다면
    {
        // 자식 본들 재귀적으로 처리
        for (int32 i = 0; i < RefSkeleton.RawRefBoneInfo.Num(); ++i)
        {
            if (RefSkeleton.RawRefBoneInfo[i].ParentIndex == BoneIndex)
            {
                RenderBoneTree(RefSkeleton, i, Engine /*, SearchFilter */); // 재귀 호출 시 Engine 전달
            }
        }
        ImGui::TreePop(); // 트리 노드 닫기
    }
    ImGui::PopID(); // ID 스택 복원
}

void SkeletalMeshViewerPanel::RenderAnimationSequence(const FReferenceSkeleton& RefSkeleton, UEditorEngine* Engine)
{
    UAnimSequence* AnimSeq = nullptr;

    if (RefSkeletalMeshComponent)
    {
        if (RefSkeletalMeshComponent->GetAnimation() && RefSkeletalMeshComponent->GetAnimationMode() == EAnimationMode::AnimationSingleNode)
        {
            AnimSeq = Cast<UAnimSequence>(RefSkeletalMeshComponent->GetAnimation());
        }
    }
    if (!AnimSeq)
    {
        return;
    }
    UAnimDataModel* DataModel = AnimSeq->GetDataModel();
    
    ImVec2 windowSize = ImVec2(Width*0.7, Height*0.3);
    ImVec2 windowPos = ImVec2(0.0f, Height - windowSize.y - 30);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    
    if (!DataModel)
    {
        return;
    }
    if (PrevAnimDataModel != DataModel)
    {
        RefSkeletalMeshComponent->SetLoopStartFrame(0);
        RefSkeletalMeshComponent->SetLoopEndFrame(FMath::Max(1, DataModel->GetNumberOfFrames() - 1));
        PrevAnimDataModel = DataModel;
    }
    
    const int32 FrameRate = DataModel->GetFrameRate();
    int32 NumFrames = DataModel->GetNumberOfFrames();
    if (NumFrames <= 1)
    {
        ImGui::Begin("Animation Sequence Timeline");
        ImGui::Text("Animation has too few frames.");
        ImGui::End();
        return;
    }

    static bool transformOpen = false;

    int32 LoopStart = RefSkeletalMeshComponent->GetLoopStartFrame();
    int32 LoopEnd = RefSkeletalMeshComponent->GetLoopEndFrame();

    LoopStart = FMath::Clamp(LoopStart, 0, NumFrames - 2);
    LoopEnd = FMath::Clamp(LoopEnd, LoopStart + 1, NumFrames - 1);

    // 유효성 확인: Start가 End보다 크거나 같거나 음수면 기본 값으로 설정
    if (LoopStart >= LoopEnd || LoopStart < 0 || LoopEnd < 0)
    {
        LoopStart = 0;
        LoopEnd = FMath::Max(1, NumFrames - 1);
        RefSkeletalMeshComponent->SetLoopStartFrame(LoopStart);
        RefSkeletalMeshComponent->SetLoopEndFrame(LoopEnd);
    }

    float PlayRate = RefSkeletalMeshComponent->GetPlayRate();
    bool bLooping = RefSkeletalMeshComponent->IsLooping();
    bool bReverse = RefSkeletalMeshComponent->IsReverse();
    bool bPlaying = RefSkeletalMeshComponent->IsPlaying();
    bool bPlayAnimation = RefSkeletalMeshComponent->bIsAnimationEnabled();

    float Elapsed = RefSkeletalMeshComponent->GetElapsedTime();
    float TargetKeyFrame = Elapsed * static_cast<float>(FrameRate);
    int32 CurrentFrame = static_cast<int32>(TargetKeyFrame) % (LoopEnd + 1);
    PreviousFrame = CurrentFrame;


    if (ImGui::Begin("Animation Sequence Timeline"))
    {
        if (ImGui::Button((!bPlayAnimation||!bPlaying)?"Play":"Pause")) {
            if (!bPlayAnimation)
            {
                RefSkeletalMeshComponent->SetPlaying(true);
            }
            else if (!bPlaying)
            {
                RefSkeletalMeshComponent->SetPlaying(true);
            }
            else
            {
                RefSkeletalMeshComponent->SetPlaying(false);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            RefSkeletalMeshComponent->SetPlaying(false);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Looping", &bLooping))
        {
            RefSkeletalMeshComponent->SetLooping(bLooping);
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Play Reverse", &bReverse))
        {
            RefSkeletalMeshComponent->SetReverse(bReverse);
        }
        ImGui::SameLine();
        FImGuiWidget::DrawDragFloat("Play Rate", PlayRate, 0.f,3.0,84);
        RefSkeletalMeshComponent->SetPlayRate(PlayRate);
        ImGui::Separator();
        ImGui::Text("Frame: %d / %d", CurrentFrame, NumFrames -1);
        ImGui::SameLine();
        ImGui::Text("Time: %.2fs", RefSkeletalMeshComponent->GetElapsedTime());
        ImGui::Separator();
        FImGuiWidget::DrawDragInt("Loop Start", LoopStart, 0, NumFrames - 2,84);
        RefSkeletalMeshComponent->SetLoopStartFrame(LoopStart);
        ImGui::SameLine();
        FImGuiWidget::DrawDragInt("Loop End", LoopEnd, LoopStart + 1, NumFrames - 1,84);
        RefSkeletalMeshComponent->SetLoopEndFrame(LoopEnd);
        
        LoopStart = FMath::Clamp(LoopStart, 0, NumFrames - 2);
        LoopEnd = FMath::Clamp(LoopEnd, LoopStart + 1, NumFrames - 1);
        if (LoopStart >= LoopEnd || LoopStart < 0 || LoopEnd < 0)
        {
            LoopStart = 0;
            LoopEnd = FMath::Max(1, NumFrames - 1);
            RefSkeletalMeshComponent->SetLoopStartFrame(LoopStart);
            RefSkeletalMeshComponent->SetLoopEndFrame(LoopEnd);
        }
        ImGui::Separator();
        if (ImGui::Button("Add New Notify Track")) {
            int32 NewTrackIdx = INDEX_NONE;
            int suffix = 0;
            FName NewTrackName;
            do {
                NewTrackName = FName(*FString::Printf(TEXT("NotifyTrack_%d"), AnimSeq->AnimNotifyTracks.Num() + suffix));
                suffix++;
            } while (AnimSeq->FindNotifyTrackIndex(NewTrackName) != INDEX_NONE);
            AnimSeq->AddNotifyTrack(NewTrackName, NewTrackIdx); 
        }
        ImGui::Separator();
        
        const TArray<FAnimNotifyTrack>& Tracks = AnimSeq->GetAnimNotifyTracks();
        const TArray<FAnimNotifyEvent>& Events = AnimSeq->Notifies;
        
        if (ImGui::BeginNeoSequencer("Sequencer", &CurrentFrame, &LoopStart, &LoopEnd,ImVec2(0,0), ImGuiNeoSequencerFlags_EnableSelection |
            ImGuiNeoSequencerFlags_Selection_EnableDragging)) {
            if (CurrentFrame != PreviousFrame)
            {
                RefSkeletalMeshComponent->SetCurrentKey(CurrentFrame);
                RefSkeletalMeshComponent->SetElapsedTime(static_cast<float>(CurrentFrame) / static_cast<float>(FrameRate));
            }

            if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                ImGui::ClearNeoKeyframeSelection();
            }

            int32 PendingRemoveTrackIdx = INDEX_NONE;
            for (int TrackIdx = 0; TrackIdx < Tracks.Num(); ++TrackIdx)
            {
                bool bOpen = true;
                bool bRenamePopup = false;
                std::string TrackLabel = *Tracks[TrackIdx].TrackName.ToString();
                FAnimNotifyTrack& CurrentTrack = AnimSeq->AnimNotifyTracks[TrackIdx];
                ImGui::PushID(TrackIdx); 
                if (ImGui::BeginNeoGroup(TrackLabel.c_str(), &bOpen))
                {
                    char trackCtxId[64];
                    if (ImGui::BeginPopupContextItem(trackCtxId)) { 
                        if (ImGui::MenuItem("Rename Track")) {
                            SelectedTrackIndex_ForRename = TrackIdx;
                            FCString::Strncpy(RenameTrackBuffer, *CurrentTrack.TrackName.ToString(), sizeof(RenameTrackBuffer) / sizeof(TCHAR) -1 );
                            RenameTrackBuffer[sizeof(RenameTrackBuffer) / sizeof(TCHAR) -1] = 0; 
                            bRenamePopup = true;
                        }
                        if (ImGui::MenuItem("Remove Track")) {
                            PendingRemoveTrackIdx = TrackIdx;
                            ImGui::CloseCurrentPopup();
                        }
                        int32 newIndex;
                        if (ImGui::MenuItem("Add Notify"))
                        {
                            AnimSeq->AddNotifyEvent(TrackIdx, Elapsed, 0, "New Notify", newIndex);
                            FAnimNotifyEvent* NotifyEvent = AnimSeq->GetNotifyEvent(newIndex);
                            if (NotifyEvent->GetNotify())
                            NotifyEvent->SetAnimNotify(FObjectFactory::ConstructObject<UAnimSoundNotify>(nullptr));
                        }
                        ImGui::EndPopup();
                    }
                    if (bRenamePopup) {
                        ImGui::OpenPopup("RenameTrackPopupModal");
                    }
                    if (ImGui::BeginPopupModal("RenameTrackPopupModal", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Text("Rename Track to:");
                        ImGui::InputText("##NewTrackNameInput", RenameTrackBuffer, sizeof(RenameTrackBuffer) / sizeof(TCHAR));
                        if (ImGui::Button("OK", ImVec2(120, 0))) {
                            if (AnimSeq->AnimNotifyTracks.IsValidIndex(SelectedTrackIndex_ForRename)) {
                                FName NewName(RenameTrackBuffer);
                                if (NewName.ToString().Len() > 0) {
                                    AnimSeq->RenameNotifyTrack(SelectedTrackIndex_ForRename, NewName);
                                }
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    if (ImGui::BeginNeoTimelineEx("Notify"))
                    {
                        for (int32 Index : Tracks[TrackIdx].NotifyIndices)
                        {
                            if (!Events.IsValidIndex(Index))
                                continue;

                            FAnimNotifyEvent& Notify = AnimSeq->Notifies[Index];

                            // 현재 Notify의 프레임 계산
                            int32 Frame = static_cast<int32>(Notify.Time * static_cast<float>(FrameRate));
                            int32 OriginalFrame = Frame;
                            float DurationFrame = Notify.Duration * static_cast<float>(FrameRate);
                            ImGui::PushID(Index);
                            if (Notify.IsState())
                            {
                                ImGui::NeoNotifyRange(&Frame, &DurationFrame , IM_COL32(255, 0, 0, 255));
                            }
                            else 
                            {
                            ImGui::NeoKeyframe(&Frame);
                            }
                            ImGui::PopID();
                            if (ImGui::IsNeoKeyframeRightClicked())
                            {
                                SelectedNotifyGlobalIndex_ForRename = Index;
                                FCString::Strncpy(RenameNotifyBuffer, *Notify.NotifyName.ToString(), sizeof(RenameNotifyBuffer) / sizeof(TCHAR) - 1);
                                RenameNotifyDuration = Notify.Duration;
                                ImGui::OpenPopup("Edit Notify");
                            }
                            // 변경 감지 후 업데이트
                            if (Frame != OriginalFrame)
                            {
                                float NewTime = static_cast<float>(Frame) / FrameRate;
                                if(Frame<LoopStart)
                                {
                                    NewTime = LoopStart / FrameRate;
                                }
                                else if(Frame + DurationFrame >LoopEnd)
                                {
                                    NewTime = (LoopEnd-DurationFrame) / FrameRate;
                                }
                                AnimSeq->UpdateNotifyEvent(Index, NewTime, Notify.Duration, Notify.TrackIndex, Notify.NotifyName);
                            }
                        }
                        ImGui::EndNeoTimeLine();
                    }
                    if (ImGui::BeginPopupModal("Edit Notify", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                    {
                        static std::string SelectedSoundName;
                        static int SoundDropdownIndex = 0;
                        
                        ImGui::Text("Rename Notify and Duration");
                        ImGui::InputText("Name", RenameNotifyBuffer, sizeof(RenameNotifyBuffer) / sizeof(TCHAR));
                        ImGui::InputFloat("Duration", &RenameNotifyDuration, 0.1f);

                        if (UAnimSoundNotify* Notify = Cast<UAnimSoundNotify>(AnimSeq->GetNotifyEvent(SelectedNotifyGlobalIndex_ForRename)->GetNotify()))
                        {
                            const auto& SoundNames = FSoundManager::GetInstance().GetAllSoundNames();
                            if (!SoundNames.IsEmpty())
                            {
                                TArray<const char*> SoundNameCStrs;
                                for (const auto& name : SoundNames)
                                    SoundNameCStrs.Add(name.c_str());

                                FName CurrentSoundName = Notify->GetSoundName();
                                for (int32 i = 0; i < SoundNames.Num(); ++i)
                                {
                                    if (FName(SoundNames[i]) == CurrentSoundName)
                                    {
                                        SoundDropdownIndex = i;
                                        break;
                                    }
                                }

                                ImGui::Text("Sound");
                                if (ImGui::Combo("##SoundCombo", &SoundDropdownIndex, SoundNameCStrs.GetData(), static_cast<int>(SoundNameCStrs.Num())))
                                {
                                    SelectedSoundName = SoundNames[SoundDropdownIndex];
                                    Notify->SetSoundName(FName(SelectedSoundName));
                                }
                            }
                        }

                        if (ImGui::Button("OK", ImVec2(120, 0)))
                        {
                            if (AnimSeq->Notifies.IsValidIndex(SelectedNotifyGlobalIndex_ForRename))
                            {
                                FName NewName(RenameNotifyBuffer);
                                float NewTime = AnimSeq->Notifies[SelectedNotifyGlobalIndex_ForRename].Time;
                                float MaxEndTime = static_cast<float>(LoopEnd) / static_cast<float>(FrameRate);
                                if ((RenameNotifyDuration + NewTime) > MaxEndTime)
                                {
                                    RenameNotifyDuration = MaxEndTime - NewTime;
                                }
                                int32 TrackIndex = AnimSeq->Notifies[SelectedNotifyGlobalIndex_ForRename].TrackIndex;

                                AnimSeq->UpdateNotifyEvent(SelectedNotifyGlobalIndex_ForRename, NewTime, RenameNotifyDuration, TrackIndex, NewName);
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Delete", ImVec2(120, 0)))
                        {
                            if (AnimSeq->Notifies.IsValidIndex(SelectedNotifyGlobalIndex_ForRename))
                            {
                                AnimSeq->RemoveNotifyEvent(SelectedNotifyGlobalIndex_ForRename);
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(120, 0)))
                        {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::EndNeoGroup();
                }
                ImGui::PopID();
            }
            if (PendingRemoveTrackIdx != INDEX_NONE && AnimSeq->AnimNotifyTracks.IsValidIndex(PendingRemoveTrackIdx))
            {
                AnimSeq->RemoveNotifyTrack(PendingRemoveTrackIdx);
            }
            ImGui::EndNeoSequencer();
        }
    }
    ImGui::End();
    
}


FString SkeletalMeshViewerPanel::GetCleanBoneName(const FString& InFullName)
{
    // 1) 계층 구분자 '|' 뒤 이름만 취하기
    int32 barIdx = InFullName.FindChar(TEXT('|'),
        /*case*/ ESearchCase::CaseSensitive,
        /*dir*/  ESearchDir::FromEnd);
    FString name = (barIdx != INDEX_NONE)
        ? InFullName.RightChop(barIdx + 1)
        : InFullName;

    // 2) 네임스페이스 구분자 ':' 뒤 이름만 취하기
    int32 colonIdx = name.FindChar(TEXT(':'),
        /*case*/ ESearchCase::CaseSensitive,
        /*dir*/  ESearchDir::FromEnd);
    if (colonIdx != INDEX_NONE)
    {
        return name.RightChop(colonIdx + 1);
    }
    return name;
}

void SkeletalMeshViewerPanel::RenderAnimationPanel(float PanelPosX, float PanelPosY, float PanelWidth, float PanelHeight)
{
    if (!RefSkeletalMeshComponent)
        return;

    EAnimationMode CurrentAnimationMode = RefSkeletalMeshComponent->GetAnimationMode();
    FString AnimModeStr = CurrentAnimationMode == EAnimationMode::AnimationBlueprint ? "Animation Instance" : "Animation Asset";

    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY + PanelHeight + 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, 250), ImGuiCond_Always);

    if (ImGui::Begin("Anim Settings", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        // Animation Mode 선택
        if (ImGui::BeginCombo("Animation Mode", GetData(AnimModeStr)))
        {
            if (ImGui::Selectable("Animation Instance", CurrentAnimationMode == EAnimationMode::AnimationBlueprint))
            {
                RefSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
                CurrentAnimationMode = EAnimationMode::AnimationBlueprint;
                RefSkeletalMeshComponent->SetAnimClass(UClass::FindClass(FName("UMyAnimInstance")));
            }
            if (ImGui::Selectable("Animation Asset", CurrentAnimationMode == EAnimationMode::AnimationSingleNode))
            {
                RefSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                CurrentAnimationMode = EAnimationMode::AnimationSingleNode;
            }
            ImGui::EndCombo();
        }
    }
    ImGui::End();
}


