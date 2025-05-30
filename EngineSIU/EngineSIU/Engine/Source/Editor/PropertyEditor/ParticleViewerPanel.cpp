#include "ParticleViewerPanel.h"

#include <wrl/module.h>

#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleSystem.h"
#include "Particles/Size/ParticleModuleSize.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/Color/ParticleModuleColorBase.h"
#include "Particles/Velocity/ParticleModuleVelocityBase.h"

ParticleViewerPanel::ParticleViewerPanel()
{
    SetSupportedWorldTypes(EWorldTypeBitFlag::ParticleViewer);
}

void ParticleViewerPanel::Render()
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine || !ParticleSystem)
    {
        return;
    }

    if (Engine->ActiveWorld && Engine->ActiveWorld->WorldType == EWorldType::ParticleViewer) 
    {
        // 컨트롤 패널 (최상단)
        RenderControlPanel();
        
        // 툴바 패널 (컨트롤 패널 아래)
        RenderToolbarPanel();
        
        // 이미터 패널 (우측)
        RenderEmitterPanel();
        
        // 디테일 패널 (좌측 하단)
        RenderDetailPanel();
        
        // Exit 버튼 (우측 하단)
        RenderExitButton();
    }
}

void ParticleViewerPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = static_cast<float>(ClientRect.right - ClientRect.left);
    Height = static_cast<float>(ClientRect.bottom - ClientRect.top);
}

void ParticleViewerPanel::RenderDetailPanel()
{
    // 디테일 패널은 좌측 하단에 배치
    float DetailPanelWidth = Width * LeftWidthSize; // 메인 뷰와 같은 너비
    float DetailPanelHeight = Height * DetailHeightSize;
    float DetailPanelPosX = LeftMargin; // 좌측에 위치 (메인 뷰와 같은 위치)
    float DetailPanelPosY = Height * (ControlHeightSize + ToolHeightSize + ViewHeightSize) + TopMargin; // 메인 뷰 아래에 위치
    
    // 패널 속성 설정
    ImGui::SetNextWindowPos(ImVec2(DetailPanelPosX, DetailPanelPosY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(DetailPanelWidth, DetailPanelHeight), ImGuiCond_Always);
    
    constexpr ImGuiWindowFlags DetailFlags = 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_HorizontalScrollbar;
    
    ImGui::Begin("Particle Details Panel", nullptr, DetailFlags);

    if (!SelectedEmitter && !SelectedModule)
    {
        ImGui::End();
        return;
    }
    
    if (SelectedEmitter)
    {
        SelectedEmitter->DisplayProperty();
    }
    
    if (SelectedModule)
    {
        ImGui::Text(GetData(SelectedModule->ModuleName.ToString()));
        SelectedModule->DisplayProperty();
    }
    
    ImGui::End();
}

void ParticleViewerPanel::RenderControlPanel() const
{
    // 컨트롤 패널은 화면 최상단에 배치
    float ControlPanelHeight = Height * ControlHeightSize;
    float ControlPanelWidth = Width;
    
    // 패널 속성 설정
    ImGui::SetNextWindowPos(ImVec2(LeftMargin, TopMargin), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ControlPanelWidth, ControlPanelHeight), ImGuiCond_Always);
    
    constexpr ImGuiWindowFlags ControlFlags = 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar;
    
    ImGui::Begin("Control Panel", nullptr, ControlFlags);
    
    // 컨트롤 패널 내용 렌더링
    ImGui::Text("Control Panel");
    
    // 여기에 컨트롤 패널의 버튼 등을 추가할 수 있습니다
    ImGui::SameLine(100.0f);
    if (ImGui::Button("Play", ImVec2(60, 30))) {
        // Play 버튼 액션
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Pause", ImVec2(60, 30))) {
        // Pause 버튼 액션
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Stop", ImVec2(60, 30))) {
        // Stop 버튼 액션
    }
    
    ImGui::End();
}

void ParticleViewerPanel::RenderToolbarPanel() const
{
    // 툴바 패널은 컨트롤 패널 아래에 배치
    float ToolbarHeight = Height * ToolHeightSize;
    float ToolbarWidth = Width;
    float ToolbarPosX = LeftMargin;
    float ToolbarPosY = Height * ControlHeightSize + TopMargin; // 컨트롤 패널 높이
    
    // 패널 속성 설정
    ImGui::SetNextWindowPos(ImVec2(ToolbarPosX, ToolbarPosY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(ToolbarWidth, ToolbarHeight), ImGuiCond_Always);
    
    constexpr ImGuiWindowFlags ToolbarFlags = 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoTitleBar;
    
    ImGui::Begin("Toolbar Panel", nullptr, ToolbarFlags);
    
    // 툴바 내용 렌더링
    ImGui::Text("Toolbar Panel");
    
    // 여기에 툴바 버튼 등을 추가할 수 있습니다
    ImGui::SameLine(200.0f);
    if (ImGui::Button("ReBuild", ImVec2(80, 30))) {
        ParticleSystemComponent->ReBuildInstancesMemoryLayout();
        // Tool 1 액션
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Tool 2", ImVec2(80, 30))) {
        // Tool 2 액션
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Tool 3", ImVec2(80, 30))) {
        // Tool 3 액션
    }
    
    ImGui::End();
}

void ParticleViewerPanel::RenderEmitterPanel()
{
    // 이미터 패널은 화면 우측에 배치
    float EmitterPanelWidth = Width * (1 - LeftWidthSize);
    float EmitterPanelHeight = Height * (ViewHeightSize + DetailHeightSize); // 컨트롤 패널 + 툴바 + 하단 여백
    float EmitterPanelPosX = Width * LeftWidthSize + LeftMargin;
    float EmitterPanelPosY = Height * (ControlHeightSize + ToolHeightSize) + TopMargin; // 컨트롤 패널 + 툴바 높이 (각각 40px)
    
    ImVec2 MinSize(200, 200);
    ImVec2 MaxSize(FLT_MAX, FLT_MAX);
    
    // 패널 속성 설정
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);
    ImGui::SetNextWindowPos(ImVec2(EmitterPanelPosX, EmitterPanelPosY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(EmitterPanelWidth, EmitterPanelHeight), ImGuiCond_Always);
    
    constexpr ImGuiWindowFlags EmitterFlags = 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_HorizontalScrollbar;
    
    ImGui::Begin("Particle Emitters", nullptr, EmitterFlags);
    
    // 이미터 패널 내용
    ImGui::Text("Emitter Panel");
    
    // 효과 세트들을 가로로 배치
    ImGui::BeginGroup();

    // 플러스 버튼을 제일 좌측에 배치
    float plusButtonWidth = 120.0f;
    float plusButtonHeight = Height * (ViewHeightSize + DetailHeightSize) * 0.9f; // 다른 이미터 세트와 같은 높이

    // 플러스 버튼 스타일 설정
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f)); // 초록색
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    
    ImGui::BeginGroup();
    // 플러스 버튼 렌더링
    if (ImGui::Button("+\nAdd\nNew\nSpriteEmitter", ImVec2(plusButtonWidth, plusButtonHeight / 2)))
    {
        // 새로운 Emitter 추가
        AddNewEmitter(EDynamicEmitterType::DET_Sprite);
    }

    if (ImGui::Button("+\nAdd\nNew\nMeshEmitter", ImVec2(plusButtonWidth, plusButtonHeight / 2)))
    {
        // 새로운 Emitter 추가
        AddNewEmitter(EDynamicEmitterType::DET_Mesh);
    }
    ImGui::EndGroup();  

    // 스타일 복원
    ImGui::PopStyleColor(3);
    
    // 기존 이미터들을 옆에 배치
    ImGui::SameLine();
    
    for (const auto& Emitter : ParticleSystem->GetEmitters())
    {
        // Emitter 한 세트
        RenderEffectSet(Emitter);
        ImGui::SameLine();
    }
    
    ImGui::EndGroup();
    
    ImGui::End();
}

void ParticleViewerPanel::AddNewEmitter(EDynamicEmitterType Type)
{
    if (!ParticleSystem)
    {
        return;
    }
    
    // 새로운 Emitter 생성
    UParticleEmitter* NewEmitter = FObjectFactory::ConstructObject<UParticleEmitter>(ParticleSystem);
    if (NewEmitter)
    {
        // 기본 이름 설정
        FString Name = "";
        if (Type == EDynamicEmitterType::DET_Sprite)
        {
            Name = "Sprite Emitter";
            NewEmitter->EmitterType = EDynamicEmitterType::DET_Sprite;
        }
        else if (Type == EDynamicEmitterType::DET_Mesh)
        {
            Name = "Mesh Emitter";
            NewEmitter->EmitterType = EDynamicEmitterType::DET_Mesh;
        }

        FString EmitterName = Name + std::to_string(ParticleSystem->GetEmitters().Num());
        NewEmitter->EmitterName = EmitterName;
        
        // ParticleSystem에 Emitter 추가
        ParticleSystem->AddEmitter(NewEmitter);

        SelectedModule = nullptr;
        SelectedEmitter = NewEmitter;
    }
}

void ParticleViewerPanel::RenderEffectSet(UParticleEmitter* Emitter)
{
    float columnWidth = 200.0f;
    
    ImGui::BeginGroup();
    
    // 큰 버튼
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
    ImGui::PushID(Emitter);
    if (ImGui::Button(GetData(Emitter->EmitterName.ToString()), ImVec2(columnWidth, 40)))
    {
        SelectedModule = nullptr;
        SelectedEmitter = Emitter;
    }
    ImGui::PopStyleVar();

    // 삭제 버튼 추가
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 0.8f));
            
    if (ImGui::Button("X##delete", ImVec2(columnWidth, 30)))
    {
        // 모듈 삭제 로직
        ParticleSystem->DeleteEmitter(Emitter);
        
        // 삭제 후 선택된 모듈 초기화(필요한 경우)
        if (SelectedEmitter == Emitter)
        {
            SelectedEmitter = nullptr;
        }
    }
    
    ImGui::PopStyleColor(3);

    // 플러스 버튼을 제일 상단에 배치
    float plusButtonHeight = 30.0f;

    // 새로운 Module 추가
    TArray<UClass*> ChildObjects;
    GetChildOfClass(UParticleModule::StaticClass(), ChildObjects);

    // 버튼 크기 설정
    ImGui::SetNextItemWidth(columnWidth);

    // 높이 조정을 위한 스타일 푸시
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, (plusButtonHeight - ImGui::GetFontSize()) * 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f)); // 드롭다운 패딩도 조정

    
    if (ImGui::BeginCombo("##AddModuleCombo", "Add Module"))
    {
        for (UClass* ChildObject : ChildObjects)
        {
            if (DisAddableClasses.Contains(ChildObject))
            {
                continue;
            }
            ImGui::PushID(ChildObject);
            FString ObjectName = ChildObject->GetName().ToAnsiString();
            if (ImGui::Selectable(GetData(ObjectName), false))
            {
                UParticleModule* NewModule = Cast<UParticleModule>(FObjectFactory::ConstructObject(ChildObject, Emitter, ObjectName));
                Emitter->GetLODLevel(0)->AddModule(NewModule);
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    // 스타일 팝
    ImGui::PopStyleVar(2);

    ImGui::PopID();

    const auto& Modules = Emitter->GetLODLevel(0)->GetModules();
    for (const auto& Module : Modules)
    {
        ImGui::PushID(Module);
        
        if (ImGui::Button(GetData(Module->ModuleName.ToString()), ImVec2(columnWidth - 60, 30)))
        {
            SelectedEmitter = nullptr;
            SelectedModule = Module;
        }

        if (!DisDeletableClasses.Contains(Module->GetClass()))
        {
            ImGui::SameLine();
            bool bChecked = Module->bEnabled;
            if (ImGui::Checkbox("", &bChecked))
            {
                Module->bEnabled = !Module->bEnabled;
            }
        }

        if (!DisDeletableClasses.Contains(Module->GetClass()))
        {
            // 삭제 버튼 추가
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 0.8f));
            
            if (ImGui::Button("X##delete", ImVec2(20, 20)))
            {
                // 모듈 삭제 로직
                Emitter->GetLODLevel(0)->DeleteModule(Module);
        
                // 삭제 후 선택된 모듈 초기화(필요한 경우)
                if (SelectedModule == Module)
                {
                    SelectedModule = nullptr;
                }
            }
    
            ImGui::PopStyleColor(3);
        }
        
        ImGui::PopID();
    }

    
    
    ImGui::EndGroup();
}

void ParticleViewerPanel::RenderExitButton() const
{
    // Exit 버튼은 우측 하단에 배치
    float ExitPanelWidth = Width * 0.2f - 6.0f;
    float ExitPanelHeight = 30.0f;
    float ExitPanelPosX = Width - ExitPanelWidth - 5.0f;
    float ExitPanelPosY = Height - ExitPanelHeight - 10.0f;
    
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
    if (ImGui::Button("Exit Viewer", ImVec2(ExitPanelWidth, ExitPanelHeight)))
    {
        UEditorEngine* EdEngine = Cast<UEditorEngine>(GEngine);
        EdEngine->EndParticleViewer();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
