local EngineTypes = EngineTypes
local CustomEnums = CustomEnums
setmetatable(_ENV, {
    __index = function(t, k)
        return rawget(EngineTypes, k) or rawget(CustomEnums, k)
    end
})

-- Template은 AActor라는 가정 하에 작동.

local ReturnTable = {} -- Return용 table. cpp에서 Table 단위로 객체 관리.

local FVector = EngineTypes.FVector -- EngineTypes로 등록된 FVector local로 선언.
local FRotator = EngineTypes.FRotator

-- 키입력을 바인딩.
function ReturnTable:InitializeCallback()
    RegisterKeyCallback("W", function(dt)
        self:MoveForward(dt)
    end)

    RegisterKeyCallback("S", function(dt)
        self:MoveBackward(dt)
    end)

    RegisterKeyCallback("A", function(dt)
        self:MoveLeft(dt)
    end)

    RegisterKeyCallback("D", function(dt)
        self:MoveRight(dt)

    end)

    RegisterMouseMoveCallback(function(dx, dy)
        -- 마우스 이동에 대한 처리
        self:Turn(dx/10)
        self:Lookup(dy/10)
        -- print("Mouse moved: ", dtX, dtY) -- 디버깅용 출력
    end)

    RegisterKeyCallback("RightMouseButton", function(dt)
        self.this:Shoot()
        -- 우클릭에 대한 처리
    end)
    
    
    RegisterKeyCallback("LeftMouseButton", function(dt)
        self.this:Punch()
        -- 우클릭에 대한 처리
    end)
    -- RegisterKeyCallback("LeftMouseButton", function(dt)
    --     self.this.State = PlayerState.Stabbing
    -- end)
end

-- BeginPlay: Actor가 처음 활성화될 때 호출
function ReturnTable:BeginPlay()

    print("BeginPlay ", self.Name) -- Table에 등록해 준 Name 출력.

end

-- Tick: 매 프레임마다 호출
function ReturnTable:Tick(DeltaTime)
    
    -- 기본적으로 Table로 등록된 변수는 self, Class usertype으로 선언된 변수는 self.this로 불러오도록 설정됨.
    -- sol::property로 등록된 변수는 변수 사용으로 getter, setter 등록이 되어 .(dot) 으로 접근가능하고
    -- 바로 등록된 경우에는 PropertyName() 과 같이 함수 형태로 호출되어야 함.
    local this = self.this
    -- this.ActorLocation = this.ActorLocation + FVector(1.0, 0.0, 0.0) * DeltaTime -- X 방향으로 이동하도록 선언.

end

-- EndPlay: Actor가 파괴되거나 레벨이 전환될 때 호출
function ReturnTable:EndPlay(EndPlayReason)
    -- print("[Lua] EndPlay called. Reason:", EndPlayReason) -- EndPlayReason Type 등록된 이후 사용 가능.
    print("EndPlay")

end

function ReturnTable:Attack(AttackDamage)
    self.GetDamate(AttackDamage)

end

function ReturnTable:MoveForward(DeltaTime)
-- print(123)    
-- print(self.this.State)
    -- self.this.State = PlayerState.Hit
    self:Move(FVector(1.0, 0.0, 0.0) * DeltaTime * 100)
end

function ReturnTable:MoveBackward(DeltaTime)
    -- self.this.State = PlayerState.Idle
    self:Move(FVector(-1.0, 0.0, 0.0) * DeltaTime * 100)
end

function ReturnTable:MoveLeft(DeltaTime)
    -- self.this.State = PlayerState.Shooting
    self:Move(FVector(0.0, -1.0, 0.0) * DeltaTime * 100)
end

function ReturnTable:MoveRight(DeltaTime)
    -- self.this.State = PlayerState.Stabbing
    self:Move(FVector(0.0, 1.0, 0.0) * DeltaTime * 100)
end

function ReturnTable:Move(dv)
    local this = self.this

    local yawOnlyRot = FRotator(0.0, this.HeadRotation.Yaw, 0.0)
    local LocalMovement = yawOnlyRot:RotateVector(dv)
    LocalMovement.Z = 0.0

    LocalMovement.X = LocalMovement.X * 6
    LocalMovement.Y = LocalMovement.Y * 6
    
    this.ActorLocation = this.ActorLocation + LocalMovement

    this.bMoving = true
end

-- 좌우 움직임
function ReturnTable:Turn(Delta)
    local this = self.this
    this.HeadRotation = this.HeadRotation + FRotator(0, Delta*5, 0)
end

-- 위아래 움직임
function ReturnTable:Lookup(Delta)
    local this = self.this
    this.HeadRotation = this.HeadRotation + FRotator(-Delta*5, 0, 0)
    print(this.HeadRotation.Pitch)
    if this.HeadRotation.Pitch < -60 then
        this.HeadRotation = FRotator(-60, this.HeadRotation.Yaw, this.HeadRotation.Roll)
    end
    if this.HeadRotation.Pitch > 80 then
        this.HeadRotation = FRotator(80, this.HeadRotation.Yaw, this.HeadRotation.Roll)
    end
    
end


return ReturnTable
