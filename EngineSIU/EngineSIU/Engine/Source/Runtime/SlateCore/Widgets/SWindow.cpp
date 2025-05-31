#include "SWindow.h"


SWindow::SWindow(FRect InRect)
    : Rect(InRect)
{}

void SWindow::Initialize(FRect InitRect)
{
    Rect = InitRect;
}

void SWindow::OnResize(uint32 InWidth, uint32 InHeight)
{
    Rect.Width = static_cast<float>(InWidth);
    Rect.Height = static_cast<float>(InHeight);
}

bool SWindow::IsHover(const FPoint& InPoint) 
{
    bIsHovered = (Rect.TopLeftX <= InPoint.x && InPoint.x < Rect.TopLeftX + Rect.Width) &&
                 (Rect.TopLeftY <= InPoint.y && InPoint.y < Rect.TopLeftY + Rect.Height);
    return bIsHovered;
}

bool SWindow::OnPressed(const FPoint& InPoint)
{
    return false;
}

bool SWindow::OnReleased() 
{
    return false;
}
