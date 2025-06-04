#include "AnimSoundNotify.h"
#include "SoundManager.h"

UAnimSoundNotify::UAnimSoundNotify()
{
    FSoundManager::GetInstance().LoadSound("footprint","Contents/Sounds/footprint.mp3");
    SoundName = "footprint";
}

void UAnimSoundNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    FSoundManager::GetInstance().PlaySound2D(*SoundName.ToString());
}

