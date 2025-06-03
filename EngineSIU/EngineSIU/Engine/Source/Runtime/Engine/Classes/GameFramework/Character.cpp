#include "Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

ACharacter::ACharacter()
    : APawn()
{
    CapsuleComponent = AddComponent<UCapsuleComponent>("CapsuleComponent");
    
    Mesh = AddComponent<USkeletalMeshComponent>("Mesh");
    Mesh->SetupAttachment(CapsuleComponent);
}

void ACharacter::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();
}

UObject* ACharacter::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    NewActor->BaseEyeHeight = BaseEyeHeight;

    for (UCapsuleComponent* Capsule : NewActor->GetComponentsByClass<UCapsuleComponent>())
    {
        if (Capsule->GetName() == CapsuleComponent->GetName())
        {
            NewActor->CapsuleComponent = Capsule;
        }
    }

    for (USkeletalMeshComponent* Comp : NewActor->GetComponentsByClass<USkeletalMeshComponent>())
    {
        if (Comp->GetName() == Mesh->GetName())
        {
            NewActor->Mesh = Comp;
        }
    }
    return NewActor;
}

void ACharacter::BeginPlay()
{
    Super::BeginPlay();

    RegisterLuaType(FLuaScriptManager::Get().GetLua());

    for (USkeletalMeshComponent* SkelComp : GetComponentsByClass<USkeletalMeshComponent>())
    {
        SkelComp->BindAnimScriptInstance(this);
    }
    // Initialize components
    //if (GetMesh())
    //{
    //    GetMesh()->InitializeComponent();
    //}

    //// Set default values for character properties
    //Health = 100.0f;
    //Stamina = 100.0f;
}

void ACharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    // Update character state
    //if (GetMesh())
    //{
    //    GetMesh()->TickComponent(DeltaTime);
    //}
    //// Handle character movement and actions
    //UpdateCharacterMovement(DeltaTime);
}

void ACharacter::Destroyed()
{
    Super::Destroyed();
    // Cleanup character resources
    //if (GetMesh())
    //{
        //GetMesh()->DestroyComponent();
    //}
}

void ACharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void ACharacter::RegisterLuaType(sol::state& Lua)
{
    DEFINE_LUA_TYPE_WITH_PARENT_ONLY(ACharacter, (sol::bases<AActor, APawn>()))
}

//FRotator ACharacter::GetMeshRotation() const
//{
//    return Mesh->GetRelativeRotation();
//}
//
//void ACharacter::SetMeshRotation(FRotator InRotation)
//{
//    return Mesh->SetRelativeRotation(InRotation);
//}
//
//FVector ACharacter::GetMeshLocation() const
//{
//    return Mesh->GetRelativeLocation();
//}
//
//void ACharacter::SetMeshLocation(FVector InLocation)
//{
//    Mesh->SetRelativeLocation(InLocation);
//}
 
