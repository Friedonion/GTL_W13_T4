#include "ControlEditorPanel.h"

#include "World/World.h"

#include "Actors/Player.h"
#include "Actors/LightActor.h"
#include "Actors/FireballActor.h"

#include "Components/Light/LightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Components/SphereComp.h"
#include "Components/ParticleSubUVComponent.h"
#include "Components/TextComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ProjectileMovementComponent.h"

#include "Engine/FObjLoader.h"
#include "Engine/StaticMeshActor.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/EditorViewportClient.h"
#include "tinyfiledialogs.h"

#include "Actors/Cube.h"

#include "Engine/EditorEngine.h"
#include <Actors/HeightFogActor.h>
#include "Actors/PointLightActor.h"
#include "Actors/DirectionalLightActor.h"
#include "Actors/SpotLightActor.h"
#include "Actors/AmbientLightActor.h"

#include "Actors/CubeActor.h"
#include "Actors/SphereActor.h"
#include "Actors/CapsuleActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Renderer/CompositingPass.h"
#include <Engine/FbxLoader.h>

#include "Animation/SkeletalMeshActor.h"
#include "Engine/Classes/Engine/AssetManager.h"
#include "Particles/ParticleSystemComponent.h"


ControlEditorPanel::ControlEditorPanel()
{
    SetSupportedWorldTypes(EWorldTypeBitFlag::Editor | EWorldTypeBitFlag::PIE | EWorldTypeBitFlag::SkeletalViewer | EWorldTypeBitFlag::PhysicsAssetViewer);
}

void ControlEditorPanel::Render()
{
    /* Pre Setup */
    const ImGuiIO& IO = ImGui::GetIO();
    ImFont* IconFont = IO.Fonts->Fonts[FEATHER_FONT];
    constexpr ImVec2 IconSize = ImVec2(32, 32);

    const float PanelWidth = (Width) * 0.8f;
    constexpr float PanelHeight = 72.0f;

    constexpr float PanelPosX = 0.0f;
    constexpr float PanelPosY = 0.0f;

    constexpr ImVec2 MinSize(300, 72);
    constexpr ImVec2 MaxSize(FLT_MAX, 72);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    constexpr ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;

    /* Render Start */
    if (!ImGui::Begin("Control Panel", nullptr, PanelFlags))
    {
        ImGui::End();
        return;
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            CreateMenuButton(IconSize, IconFont);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("ImGui"))
        {
            if (ImGui::MenuItem("ImGui Demo"))
            {
                bShowImGuiDemoWindow = true;
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
    
    if (bOpenModal)
    {
        const ImVec2 Center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
        ImGui::OpenPopup("##Application Quit", ImGuiPopupFlags_AnyPopup);
        if (ImGui::BeginPopupModal("##Application Quit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("정말 프로그램을 종료하시겠습니까?");
            ImGui::Separator();

            const float ContentWidth = ImGui::GetWindowContentRegionMax().x;
            /* Move Cursor X Position */
            ImGui::SetCursorPosX(ContentWidth - (160.f + 10.0f));
            if (ImGui::Button("OK", ImVec2(80, 0)))
            {
                PostQuitMessage(0);
            }
            ImGui::SameLine();
            ImGui::SetItemDefaultFocus();
            ImGui::PushID("CancelButtonWithQuitWindow");
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(ImColor::HSV(0.0f, 1.0f, 1.0f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(ImColor::HSV(0.0f, 0.9f, 1.0f)));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(ImColor::HSV(0.0f, 1.0f, 1.0f)));
            if (ImGui::Button("Cancel", ImVec2(80, 0)))
            {
                ImGui::CloseCurrentPopup();
                bOpenModal = false;
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();

            ImGui::EndPopup();
        }
    }

    ImGui::SameLine();
    CreateFlagButton();
    
    ImGui::SameLine();
    CreateModifyButton(IconSize, IconFont);

    //ImGui::SameLine();
    // CreateLightSpawnButton(IconSize, IconFont);

    ImGui::SameLine();
    {
        ImGui::PushFont(IconFont);
        CreatePIEButton(IconSize, IconFont);
        ImGui::SameLine();
        /* Get Window Content Region */
        const float ContentWidth = ImGui::GetWindowContentRegionMax().x;
        /* Move Cursor X Position */
        if (Width >= 880.f)
        {
            ImGui::SetCursorPosX(ContentWidth - (IconSize.x * 3.0f + 16.0f));
        }
        CreateSRTButton(IconSize);
        ImGui::PopFont();
    }

    ImGui::End();

    if (bShowImGuiDemoWindow)
    {
        ImGui::ShowDemoWindow(&bShowImGuiDemoWindow); // 창이 닫힐 때 상태를 업데이트
    }
}

void ControlEditorPanel::CreateMenuButton(const ImVec2 ButtonSize, ImFont* IconFont)
{
    if (GEngine->ActiveWorld->WorldType == EWorldType::SkeletalViewer)
    {
        return;
    }

    if (ImGui::MenuItem("New Level"))
    {
        if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            EditorEngine->NewLevel();
        }
    }

    if (ImGui::MenuItem("Load Level"))
    {
        char const* lFilterPatterns[1] = { "*.scene" };
        const char* FileName = tinyfd_openFileDialog("Open Scene File", "", 1, lFilterPatterns, "Scene(.scene) file", 0);

        if (FileName == nullptr)
        {
            tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
            ImGui::End();
            return;
        }
        if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            EditorEngine->NewLevel();
            EditorEngine->LoadLevel(FileName);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Save Level"))
    {
        char const* lFilterPatterns[1] = { "*.scene" };
        const char* FileName = tinyfd_saveFileDialog("Save Scene File", "", 1, lFilterPatterns, "Scene(.scene) file");

        if (FileName == nullptr)
        {
            ImGui::End();
            return;
        }
        if (const UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            EditorEngine->SaveLevel(FileName);
        }

        tinyfd_messageBox("알림", "저장되었습니다.", "ok", "info", 1);
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("Import"))
    {
        if (ImGui::MenuItem("Wavefront (.obj)"))
        {
            char const* lFilterPatterns[1] = { "*.obj" };
            const char* FileName = tinyfd_openFileDialog("Open OBJ File", "", 1, lFilterPatterns, "Wavefront(.obj) file", 0);

            if (FileName != nullptr)
            {
                std::cout << FileName << '\n';

                if (FObjManager::CreateStaticMesh(FileName) == nullptr)
                {
                    tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                }
            }
        }

        ImGui::EndMenu();
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Quit"))
    {
        bOpenModal = true;
    }
}

void ControlEditorPanel::CreateModifyButton(const ImVec2 ButtonSize, ImFont* IconFont)
{
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9c4", ButtonSize)) // Slider
    {
        ImGui::OpenPopup("SliderControl");
    }
    ImGui::PopFont();

    if (ImGui::BeginPopup("SliderControl"))
    {
        ImGui::Text("Grid Scale");
        GridScale = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetGridSize();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Grid Scale", &GridScale, 0.1f, 1.0f, 20.0f, "%.1f"))
        {
            GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SetGridSize(GridScale);
        }

        ImGui::Separator();

        ImGui::Text("Camera FOV");
        FOV = &GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->ViewFOV;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Fov", FOV, 0.1f, 30.0f, 120.0f, "%.1f"))
        {
            //GEngineLoop.GetWorld()->GetCamera()->SetFOV(FOV);
        }

        ImGui::Spacing();

        ImGui::Text("Camera Speed");
        CameraSpeed = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetCameraSpeedScalar();
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##CamSpeed", &CameraSpeed, 0.1f, 0.198f, 192.0f, "%.1f"))
        {
            GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SetCameraSpeed(CameraSpeed);
        }

        ImGui::Text("F-Stop");
        float F_Stop = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->F_Stop;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##F-Stop", &F_Stop, 0.01f, 1.2f, 32.0f, "%.01f"))
        {
            GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->F_Stop = F_Stop;
        }

        ImGui::Spacing();

        ImGui::Text("Sensor Width");
        float SensorWidth = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SensorWidth;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##SensorWidth", &SensorWidth, 0.01f, 0.1f, 300.0f, "%.01f"))
        {
            GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->SensorWidth = SensorWidth;
        }

        ImGui::Spacing();

        ImGui::Text("Focal Distance");
        float FocalDistance = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->FocalDistance;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##FocalDistance", &FocalDistance, 0.1f, 0.0f, 10000.0f, "%.1f"))
        {
            GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->FocalDistance = FocalDistance;
        }

        ImGui::Spacing();

        ImGui::Separator();

        ImGui::Text("Gamma");
        float Gamma = FEngineLoop::Renderer.CompositingPass->GammaValue;
        ImGui::SetNextItemWidth(120.0f);
        if (ImGui::DragFloat("##Gamma", &Gamma, 0.01f, 0.01f, 4.0f, "%.1f"))
        {
            FEngineLoop::Renderer.CompositingPass->GammaValue = Gamma;
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // @todo 적절한 이름으로 변경 바람
    ImGui::PushFont(IconFont);
    if (ImGui::Button("\ue9c8", ButtonSize))
    {
        ImGui::OpenPopup("PrimitiveControl");
    }
    ImGui::PopFont();

    if (ImGui::BeginPopup("PrimitiveControl"))
    {
        struct Primitive
        {
            const char* Label;
            int OBJ;
        };

        static const Primitive primitives[] = 
        {
            { .Label = "Cube",              .OBJ = OBJ_CUBE },
            { .Label = "Sphere",            .OBJ = OBJ_SPHERE },
            { .Label = "PointLight",        .OBJ = OBJ_POINTLIGHT },
            { .Label = "SpotLight",         .OBJ = OBJ_SPOTLIGHT },
            { .Label = "DirectionalLight",  .OBJ = OBJ_DIRECTIONALLGIHT },
            { .Label = "AmbientLight",      .OBJ = OBJ_AMBIENTLIGHT },
            { .Label = "Particle",          .OBJ = OBJ_PARTICLE },
            { .Label = "ParticleSystem",    .OBJ = OBJ_PARTICLESYSTEM },
            { .Label = "Text",              .OBJ = OBJ_TEXT },
            { .Label = "Fog",               .OBJ = OBJ_FOG },
            { .Label = "BoxCol",            .OBJ = OBJ_BOX_COLLISION },
            { .Label = "SphereCol",         .OBJ = OBJ_SPHERE_COLLISION },
            { .Label = "CapsuleCol",        .OBJ = OBJ_CAPSULE_COLLISION },
            { .Label = "SkeletalMeshActor", .OBJ = OBJ_SKELETALMESH },
            { .Label = "SequencerPlayer",   .OBJ = OBJ_SEQUENCERPLAYER },
        };

        for (const auto& primitive : primitives)
        {
            if (ImGui::Selectable(primitive.Label))
            {
                UWorld* World = GEngine->ActiveWorld;
                AActor* SpawnedActor = nullptr;
                switch (static_cast<OBJECTS>(primitive.OBJ))
                {
                case OBJ_SPHERE:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_SPHERE"));
                    USphereComp* SphereComp = SpawnedActor->AddComponent<USphereComp>();
                    SphereComp->SetStaticMesh(FObjManager::GetStaticMesh(L"Contents/Sphere.obj"));
                    break;
                }
                case OBJ_CUBE:
                {
                    // TODO: 다른 부분들 전부 Actor만 소환하도록 하고, Component 생성은 Actor가 자체적으로 하도록 변경.
                    ACube* CubeActor = World->SpawnActor<ACube>();
                    CubeActor->SetActorLabel(TEXT("OBJ_CUBE"));
                    break;
                }

                case OBJ_SPOTLIGHT:
                {
                    ASpotLight* SpotActor = World->SpawnActor<ASpotLight>();
                    SpotActor->SetActorLabel(TEXT("OBJ_SPOTLIGHT"));
                    break;
                }
                case OBJ_POINTLIGHT:
                {
                    APointLight* LightActor = World->SpawnActor<APointLight>();
                    LightActor->SetActorLabel(TEXT("OBJ_POINTLIGHT"));
                    break;
                }
                case OBJ_DIRECTIONALLGIHT:
                {
                    ADirectionalLight* LightActor = World->SpawnActor<ADirectionalLight>();
                    LightActor->SetActorLabel(TEXT("OBJ_DIRECTIONALLGIHT"));
                    break;
                }
                case OBJ_AMBIENTLIGHT:
                {
                    AAmbientLight* LightActor = World->SpawnActor<AAmbientLight>();
                    LightActor->SetActorLabel(TEXT("OBJ_AMBIENTLIGHT"));
                    break;
                }
                case OBJ_PARTICLE:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_PARTICLE"));
                    UParticleSubUVComponent* ParticleComponent = SpawnedActor->AddComponent<UParticleSubUVComponent>();
                    ParticleComponent->SetTexture(L"Assets/Texture/T_Explosion_SubUV.png");
                    ParticleComponent->SetRowColumnCount(6, 6);
                    ParticleComponent->SetRelativeScale3D(FVector(10.0f, 10.0f, 1.0f));
                    ParticleComponent->Activate();
                    SpawnedActor->SetActorTickInEditor(true);
                    break;
                }
                case OBJ_PARTICLESYSTEM:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_PARTICLESYSTEM"));
                    UParticleSystemComponent* ParticleComponent = SpawnedActor->AddComponent<UParticleSystemComponent>();
                    SpawnedActor->SetRootComponent(ParticleComponent);
                    SpawnedActor->SetActorTickInEditor(true);
                    break;
                }
                case OBJ_TEXT:
                {
                    SpawnedActor = World->SpawnActor<AActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_TEXT"));
                    UTextComponent* TextComponent = SpawnedActor->AddComponent<UTextComponent>();
                    TextComponent->SetTexture(L"Assets/Texture/font.png");
                    TextComponent->SetRowColumnCount(106, 106);
                    TextComponent->SetText(L"Default");
                    SpawnedActor->SetRootComponent(TextComponent);
                    
                    break;
                }
                case OBJ_FOG:
                {
                    SpawnedActor = World->SpawnActor<AHeightFogActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_FOG"));
                    break;
                }
                case OBJ_BOX_COLLISION:
                {
                    SpawnedActor = World->SpawnActor<ACubeActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_BOX_COLLISION"));
                    SpawnedActor->SetActorTickInEditor(true); // TODO: 콜리전 테스트 용도
                    break;
                }
                case OBJ_SPHERE_COLLISION:
                {
                    SpawnedActor = World->SpawnActor<ASphereActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_SPHERE_COLLISION"));
                    SpawnedActor->SetActorTickInEditor(true); // TODO: 콜리전 테스트 용도
                    break;
                }
                case OBJ_CAPSULE_COLLISION:
                {
                    SpawnedActor = World->SpawnActor<ACapsuleActor>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_CAPSULE_COLLISION"));
                    SpawnedActor->SetActorTickInEditor(true); // TODO: 콜리전 테스트 용도
                    break;
                }
                case OBJ_SKELETALMESH:
                    {
                        SpawnedActor = World->SpawnActor<ASkeletalMeshActor>();
                        SpawnedActor->SetActorTickInEditor(true);
                    }
                    break;
                case OBJ_SEQUENCERPLAYER:
                {
                    SpawnedActor = World->SpawnActor<ASequencerPlayer>();
                    SpawnedActor->SetActorLabel(TEXT("OBJ_SEQUENCERPLAYER"));
                }
                case OBJ_CAMERA:
                case OBJ_PLAYER:
                case OBJ_END:
                    break;
                }

                if (SpawnedActor)
                {
                    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
                    Engine->DeselectComponent(Engine->GetSelectedComponent());
                    Engine->SelectActor(SpawnedActor);
                }
            }
        }
        ImGui::EndPopup();
    }
}

void ControlEditorPanel::CreateFlagButton()
{
    const std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();

    const char* ViewTypeNames[] = { "Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back" };
    const ELevelViewportType ActiveViewType = ActiveViewport->GetViewportType();
    FString TextViewType = ViewTypeNames[ActiveViewType];

    if (ImGui::Button(GetData(TextViewType), ImVec2(120, 32)))
    {
        // toggleViewState = !toggleViewState;
        ImGui::OpenPopup("ViewControl");
    }

    if (ImGui::BeginPopup("ViewControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewTypeNames); i++)
        {
            bool bIsSelected = (static_cast<int>(ActiveViewport->GetViewportType()) == i);
            if (ImGui::Selectable(ViewTypeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewportType(static_cast<ELevelViewportType>(i));
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    const char* ViewModeNames[] = { 
        "Lit_Gouraud", "Lit_Lambert", "Lit_Blinn-Phong", "Lit_PBR",
        "Unlit", "Wireframe",
        "Scene Depth", "World Normal", "World Tangent","Light Heat Map"
    };
    constexpr uint32 ViewModeCount = std::size(ViewModeNames);
    
    const int RawViewMode = static_cast<int>(ActiveViewport->GetViewMode());
    const int SafeIndex = (RawViewMode >= 0) ? (RawViewMode % ViewModeCount) : 0;
    FString ViewModeControl = ViewModeNames[SafeIndex];
    
    const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(GetData(ViewModeControl));
    if (ImGui::Button(GetData(ViewModeControl), ImVec2(30 + ViewModeTextSize.x, 32)))
    {
        ImGui::OpenPopup("ViewModeControl");
    }

    if (ImGui::BeginPopup("ViewModeControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewModeNames); i++)
        {
            bool bIsSelected = (static_cast<int>(ActiveViewport->GetViewMode()) == i);
            if (ImGui::Selectable(ViewModeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewMode(static_cast<EViewModeIndex>(i));
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ShowFlags::GetInstance().Draw(ActiveViewport);
}

void ControlEditorPanel::CreatePIEButton(const ImVec2 ButtonSize, ImFont* IconFont)
{
    if (GEngine->ActiveWorld->WorldType == EWorldType::SkeletalViewer)
    {
        return;
    }
    
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    if (!Engine)
    {
        return;
    }

    const float WindowSizeX = Width * 0.8f;
    const float CenterX = WindowSizeX * 0.5f - ButtonSize.x;
    
    if (Width >= 1200.f)
    {
        ImGui::SetCursorPosX(CenterX - ButtonSize.x * 0.5f);
    }
    
    if (ImGui::Button("\ue9a8", ButtonSize)) // Play
    {
        UE_LOG(ELogLevel::Display, TEXT("PIE Button Clicked"));
        Engine->StartPIE();
    }
    ImGui::SameLine();

    if (Width >= 1200.f)
    {
        ImGui::SetCursorPosX(CenterX + ButtonSize.x * 0.5f);
    }
    
    if (ImGui::Button("\ue9e4", ButtonSize)) // Stop
    {
        UE_LOG(ELogLevel::Display, TEXT("Stop Button Clicked"));
        Engine->EndPIE();
    }
}

// code is so dirty / Please refactor
void ControlEditorPanel::CreateSRTButton(ImVec2 ButtonSize)
{
    const UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);
    AEditorPlayer* Player = Engine->GetEditorPlayer();

    constexpr ImVec4 ActiveColor = ImVec4(0.00f, 0.30f, 0.00f, 1.0f);

    const EControlMode ControlMode = Player->GetControlMode();

    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9bc", ButtonSize)) // Move
    {
        Player->SetMode(CM_TRANSLATION);
    }
    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ControlMode == CM_ROTATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9d3", ButtonSize)) // Rotate
    {
        Player->SetMode(CM_ROTATION);
    }
    if (ControlMode == CM_ROTATION)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ControlMode == CM_SCALE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9ab", ButtonSize)) // Scale
    {
        Player->SetMode(CM_SCALE);
    }
    if (ControlMode == CM_SCALE)
    {
        ImGui::PopStyleColor();
    }
}

void ControlEditorPanel::OnResize(const HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void ControlEditorPanel::CreateLightSpawnButton(const ImVec2 InButtonSize, ImFont* IconFont)
{
    if (GEngine->ActiveWorld->WorldType == EWorldType::SkeletalViewer)
    {
        return;
    }
    UWorld* World = GEngine->ActiveWorld;
    const ImVec2 WindowSize = ImGui::GetIO().DisplaySize;

    const float CenterX = (WindowSize.x - InButtonSize.x) / 2.5f;

    ImGui::SetCursorScreenPos(ImVec2(CenterX + 40.0f, 10.0f));
    const char* Text = "Light";
    const ImVec2 TextSize = ImGui::CalcTextSize(Text);
    const ImVec2 Padding = ImGui::GetStyle().FramePadding;
    ImVec2 ButtonSize = ImVec2(
        TextSize.x + Padding.x * 2.0f,
        TextSize.y + Padding.y * 2.0f
    );
    ButtonSize.y = InButtonSize.y;
    if (ImGui::Button("Light", ButtonSize))
    {
        ImGui::OpenPopup("LightGeneratorControl");
    }

    if (ImGui::BeginPopup("LightGeneratorControl"))
    {
        struct LightGeneratorMode
        {
            const char* Label;
            int Mode;
        };

        static constexpr LightGeneratorMode modes[] = 
        {
            {.Label = "Generate", .Mode = ELightGridGenerator::Generate },
            {.Label = "Delete", .Mode = ELightGridGenerator::Delete },
            {.Label = "Reset", .Mode = ELightGridGenerator::Reset },
        };

        for (const auto& [Label, Mode] : modes)
        {
            if (ImGui::Selectable(Label))
            {
                switch (Mode)
                {
                case ELightGridGenerator::Generate:
                    LightGridGenerator.GenerateLight(World);
                    break;
                case ELightGridGenerator::Delete:
                    LightGridGenerator.DeleteLight(World);
                    break;
                case ELightGridGenerator::Reset:
                    LightGridGenerator.Reset(World);
                    break;
                }
            }
        }

        ImGui::EndPopup();
    }
}
