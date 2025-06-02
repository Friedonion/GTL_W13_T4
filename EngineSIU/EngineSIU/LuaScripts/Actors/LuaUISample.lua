
setmetatable(_ENV, { __index = EngineTypes })

-- Template은 AActor라는 가정 하에 작동.

local ReturnTable = {} -- Return용 table. cpp에서 Table 단위로 객체 관리.

local FVector = EngineTypes.FVector -- EngineTypes로 등록된 FVector local로 선언.

local LuaUIBind = EngineTypes.LuaUIBind
local RectTransform = EngineTypes.RectTransform
local FString = EngineTypes.FString
local AnchorDirection = EngineTypes.AnchorDirection
local FLinearColor = EngineTypes.FLinearColor

local LuaTextUI = EngineTypes.LuaTextUI
local LuaImageUI = EngineTypes.LuaImageUI

-- BeginPlay: Actor가 처음 활성화될 때 호출
function ReturnTable:BeginPlay()

    print("BeginPlay ", self.Name) -- Table에 등록해 준 Name 출력.

    print("BeginPlay ", self.Name) -- Table에 등록해 준 Name 출력.

    self.ManagedTextUI = nil
    self.ManagedImageUI = nil
    self.ElapsedTime = 0 -- 시간 누적용

    
    local uiName = FString.new("MyDynamicTextUI")
    local textContent = FString.new("Hello from Lua with Dynamic Rect!")

    local posX = -150.0
    local posY = 50.0
    local width = 200.0     -- Text는 FontSize에만 크기 영향 받음
    local height = 80.0
    local anchor = AnchorDirection.TopCenter -- Enum 값 사용

    local myRect = RectTransform.new(posX, posY, width, height, anchor)

    local SortOrder = 20
    local fontName = FString.new("Default")
    local fontSize = 50
    local fontColor = FLinearColor.new(1.0, 0.0, 1.0, 1.0)

    LuaUIBind.CreateText(uiName,  myRect, SortOrder, textContent, fontName, fontSize, fontColor)

    local imageName = FString.new("MyImageUI")
    local textureName = FString.new("ExplosionColor")

    local TexturePosX = -450.0
    local TexturePosY = 50.0
    local TextureWidth = 300.0
    local TextureHeight = 80.0
    local TextureAnchor = AnchorDirection.MiddleCenter

    local TextureRect = RectTransform.new(TexturePosX, TexturePosY, TextureWidth, TextureHeight, TextureAnchor)

    local TextureSortOrder = 10
    local TextureColor = FLinearColor.new(1.0, 1.0, 1.0, 0.1)

    LuaUIBind.CreateImage(imageName, TextureRect, TextureSortOrder, textureName, TextureColor)


    -- 생성된 UI 객체 가져오기 (Tick에서 사용하기 위해)
    -- LuaUIBind.GetTextUI/GetImageUI는 포인터를 반환하므로, Lua에서 객체로 다뤄짐
    self.ManagedTextUI = LuaUIBind.GetTextUI(uiName) -- **중요: CreateText의 첫번째 인자(고유 이름)로 Get 해야함**
    self.ManagedImageUI = LuaUIBind.GetImageUI(imageName)

    -- 객체를 제대로 가져왔는지 확인 (nil 체크)
    if not self.ManagedTextUI then
        print("Error: Could not get ManagedTextUI with name: " .. uiName:ToAnsiString())
    end
    if not self.ManagedImageUI then
        print("Error: Could not get ManagedImageUI with name: " .. imageName:ToAnsiString())
    end

end

-- Tick: 매 프레임마다 호출
function ReturnTable:Tick(DeltaTime)
    
    -- 기본적으로 Table로 등록된 변수는 self, Class usertype으로 선언된 변수는 self.this로 불러오도록 설정됨.
    -- sol::property로 등록된 변수는 변수 사용으로 getter, setter 등록이 되어 .(dot) 으로 접근가능하고
    -- 바로 등록된 경우에는 PropertyName() 과 같이 함수 형태로 호출되어야 함.
    local this = self.this
    -- this.ActorLocation = this.ActorLocation + FVector(1.0, 0.0, 0.0) * DeltaTime -- X 방향으로 이동하도록 선언.
    self.ElapsedTime = self.ElapsedTime + DeltaTime

    -- Alpha 값을 0에서 1 사이로 부드럽게 변화 (sin 함수 사용)
    -- (math.sin(x) + 1) / 2  => 결과 범위 [0, 1]
    -- self.ElapsedTime * speed_multiplier 로 속도 조절 가능
    local speed = 1.0 -- Alpha 변화 속도 (값이 클수록 빠름)
    local calculatedAlpha = (math.sin(self.ElapsedTime * speed) + 1.0) / 2.0

    -- Text UI Alpha 업데이트
    if self.ManagedTextUI then
        local currentTextColor = self.ManagedTextUI.FontColor -- LUA_BIND_MEMBER(&LuaTextUI::FontColor)로 읽기 가능 가정
        if currentTextColor then
            -- 새 FLinearColor 객체를 만들어서 Alpha만 변경 후 설정
            local newTextColor = FLinearColor.new(currentTextColor.R, currentTextColor.G, currentTextColor.B, calculatedAlpha)
            self.ManagedTextUI:SetFontColor(newTextColor)
        else
             print("Warning: Could not get currentTextColor from ManagedTextUI.FontColor")
             -- 임시로 고정 RGB 값 사용
             local fallbackTextColor = FLinearColor.new(1.0, 0.0, 1.0, calculatedAlpha) -- 초기값과 동일한 RGB
             self.ManagedTextUI:SetFontColor(fallbackTextColor)
        end
    end

    -- Image UI Alpha 업데이트
    if self.ManagedImageUI then
        -- Text UI와 유사하게 처리. LuaImageUI.Color를 읽거나, SetColor가 RGB를 유지한다고 가정.
        local currentImageColor = self.ManagedImageUI.Color -- LUA_BIND_MEMBER(&LuaImageUI::Color)로 읽기 가능 가정
        if currentImageColor then
            local newImageColor = FLinearColor.new(currentImageColor.R, currentImageColor.G, currentImageColor.B, calculatedAlpha)
            self.ManagedImageUI:SetColor(newImageColor)
        else
            print("Warning: Could not get currentImageColor from ManagedImageUI.Color")
            -- 임시로 고정 RGB 값 사용
            local fallbackImageColor = FLinearColor.new(1.0, 1.0, 1.0, calculatedAlpha) -- 초기값과 동일한 RGB
            self.ManagedImageUI:SetColor(fallbackImageColor)
        end
    end

    if self.ElapsedTime >= 3.0 then
        LuaUIBind.DeleteUI(self.ManagedTextUI:GetNameStr())
        self.ManagedTextUI = nil
    end

end

-- EndPlay: Actor가 파괴되거나 레벨이 전환될 때 호출
function ReturnTable:EndPlay(EndPlayReason)
    -- print("[Lua] EndPlay called. Reason:", EndPlayReason) -- EndPlayReason Type 등록된 이후 사용 가능.
    print("EndPlay")

end

function ReturnTable:Attack(AttackDamage)
    self.GetDamate(AttackDamage)

end

return ReturnTable
