#pragma once
#include "Pawn.h"

class UCapsuleComponent;
class USkeletalMeshComponent;

class ACharacter : public APawn
{
    DECLARE_CLASS(ACharacter, APawn)

public:
    ACharacter() = default;
    

private:

    UPROPERTY(
        VisibleAnywhere,
        USkeletalMeshComponent*,
        Mesh,
        = nullptr
    )

    UPROPERTY(
        VisibleAnywhere,
        UCapsuleComponent*,
        CapsuleComponent,
        = nullptr
    )

};
