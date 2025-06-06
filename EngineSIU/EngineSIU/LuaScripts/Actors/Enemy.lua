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

-- BeginPlay: Actor가 처음 활성화될 때 호출
function ReturnTable:BeginPlay()

    print("BeginPlay ", self.Name) -- Table에 등록해 준 Name 출력.

end

-- Tick: 매 프레임마다 호출
function ReturnTable:TickPatrol(DeltaTime)
    print(123123123123)
    -- 기본적으로 Table로 등록된 변수는 self, Class usertype으로 선언된 변수는 self.this로 불러오도록 설정됨.
    -- sol::property로 등록된 변수는 변수 사용으로 getter, setter 등록이 되어 .(dot) 으로 접근가능하고
    -- 바로 등록된 경우에는 PropertyName() 과 같이 함수 형태로 호출되어야 함.
    local this = self.this
    -- this.ActorLocation = this.ActorLocation + FVector(1.0, 0.0, 0.0) * DeltaTime -- X 방향으로 이동하도록 선언.

    local Current = this.ActorLocation
    
    local Region1 = this.PatrolStartLocation + this.PatrolDirection
    local Region2 = this.PatrolStartLocation - this.PatrolDirection

    local FarFromStart = Current - this.PatrolStartLocation
    local FarDistance = FarFromStart:Length()
    local MaxDistancce = this.PatrolDirection:Length() * 10

    print(this.MoveDirection.X)
    print(this.MoveDirection.Y)
    print(this.MoveDirection.Z)

    this.MoveDirection.X = this.MoveDirection.X * DeltaTime
    this.MoveDirection.Y = this.MoveDirection.Y * DeltaTime
    this.MoveDirection.Z = this.MoveDirection.Z * DeltaTime
        print(this.ActorLocation.X)
    print(this.ActorLocation.Y)
    print(this.ActorLocation.Z)

    this.ActorLocation = Current + this.MoveDirection
    print(DeltaTime)
    print(Current.X)
    print(Current.Y)
    print(Current.Z)
    print(this.ActorLocation.X)
    print(this.ActorLocation.Y)
    print(this.ActorLocation.Z)
    print(this.MoveDirection.X)
    print(this.MoveDirection.Y)
    print(this.MoveDirection.Z)
    -- this.ActorLocation = this.ActorLocation + FVector(0.3, 0, 0)
    
end

-- EndPlay: Actor가 파괴되거나 레벨이 전환될 때 호출
function ReturnTable:EndPlay(EndPlayReason)
    -- print("[Lua] EndPlay called. Reason:", EndPlayReason) -- EndPlayReason Type 등록된 이후 사용 가능.
    print("EndPlay")

end

return ReturnTable
