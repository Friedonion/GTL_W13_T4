#pragma once
#include "Actor.h"
#include "Bullet.h"

class ABulletTester :    public AActor
{
    DECLARE_CLASS(ABulletTester, AActor)

public:
    ABulletTester();
    virtual ~ABulletTester();

    // BeginPlay에서 딱 한 번만 총알을 스폰하고 발사합니다.
    virtual void BeginPlay() override;

private:
    // C++ 레벨에서 ABullet::StaticClass()를 기본값으로 하되,
    // 에디터를 사용한다면 여기 값을 바꿔서 다른 서브클래스를 지정할 수도 있습니다.
    UClass* BulletClass;

    // 한 번만 발사했는지 체크하는 플래그
    bool bHasFired;
};
