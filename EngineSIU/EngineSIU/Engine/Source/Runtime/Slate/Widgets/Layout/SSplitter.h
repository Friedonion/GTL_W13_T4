#pragma once
#include "SlateCore/Widgets/SWindow.h"
#include "Container/Map.h"
#include "fstream"
#include "sstream"

class SSplitter : public SWindow
{
public:
    SWindow* SideLT = nullptr; // Left or Top
    SWindow* SideRB = nullptr; // Right or Bottom

    float SplitRatio = 0.5f; // 값 범위: [0, 1]

    uint32 SplitterLimitLT = 100; // Pixel Value

    uint32 SplitterHalfThickness = 5; // Pixel Value

    bool bIsSplitterPressed = false;

    virtual void Initialize(FRect InitRect) override;

    virtual void ClampSplitRatio() {}
    
    virtual float GetSplitterLTCenter() = 0;

    virtual void OnDragStart(const FPoint& MousePos) { /* 초기화 */ }
    virtual void OnDrag(const FPoint& Delta) = 0; // 가로/세로에 따라 구현 다름.
    virtual void OnResize(uint32 InWidth, uint32 InHeight) override;
    virtual bool OnPressed(const FPoint& InPoint) override;
    virtual bool OnReleased() override;

    virtual bool IsSplitterHovered(const FPoint& InPoint) const;

    virtual bool IsSplitterPressed() const { return bIsSplitterPressed; }

    virtual void OnDragEnd() {}

    virtual void LoadConfig(const TMap<FString, FString>& Config);
    virtual void SaveConfig(TMap<FString, FString>& Config) const;

    // 스플리터가 포함된 영역에 따라 자식 창의 Rect를 재계산하는 함수
    virtual void UpdateChildRects() = 0;

    template <typename T>
    T GetValueFromConfig(const TMap<FString, FString>& Config, const FString& Key, T DefaultValue) {
        if (const FString* Value = Config.Find(Key))
        {
            std::istringstream Stream(**Value);
            T ConfigValue;
            if (Stream >> ConfigValue)
            {
                return ConfigValue;
            }
        }
        return DefaultValue;
    }
};

class SSplitterH : public SSplitter
{
public:
    virtual void Initialize(FRect InRect) override;

    virtual void ClampSplitRatio() override;
    
    virtual float GetSplitterLTCenter() override;
    
    virtual void OnResize(uint32 InWidth, uint32 InHeight) override;

    virtual void LoadConfig(const TMap<FString, FString>& Config) override;
    virtual void SaveConfig(TMap<FString, FString>& Config) const override;

    virtual void OnDrag(const FPoint& Delta) override;

    virtual void UpdateChildRects() override;
};

class SSplitterV : public SSplitter
{
public:
    virtual void Initialize(FRect InRect) override;

    virtual void ClampSplitRatio() override;

    virtual float GetSplitterLTCenter() override;
    
    virtual void OnResize(uint32 InWidth, uint32 InHeight) override;

    virtual void LoadConfig(const TMap<FString, FString>& Config) override;
    virtual void SaveConfig(TMap<FString, FString>& Config) const override;

    virtual void OnDrag(const FPoint& Delta) override;

    virtual void UpdateChildRects() override;
};
