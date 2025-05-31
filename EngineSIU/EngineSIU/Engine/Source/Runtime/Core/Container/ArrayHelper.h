#pragma once
#include "Array.h"
#include "Serialization/Archive.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"

struct FArrayHelper
{
    template <typename T, typename AllocatorType>
    static void SerializePtrAsset(FArchive& Ar, TArray<T, AllocatorType>& Array, UObject* Outer = nullptr)
    {
        static_assert(std::is_pointer_v<T>, "T must be a pointer type for SerializePtrAsset.");
    
        using PointerToType = std::remove_pointer_t<T>;
        using SizeType = typename TArray<T, AllocatorType>::SizeType;
        constexpr bool bIsUObject = std::is_base_of_v<UObject, PointerToType>;
    
        uint32 ArraySize = static_cast<uint32>(Array.Num());
        Ar << ArraySize;

        if (Ar.IsLoading())
        {
            // 기존에 존재하던 원소 모두 제거
            if constexpr (bIsUObject)
            {
                for (SizeType Index = 0; Index < Array.Num(); ++Index)
                {
                    Array.ContainerPrivate[Index]->MarkAsGarbage();
                }
            }
            else
            {
                for (SizeType Index = 0; Index < Array.Num(); ++Index)
                {
                    delete Array.ContainerPrivate[Index];
                }
            }

            Array.Empty();
            Array.SetNum(ArraySize);
        }

        for (uint32 Index = 0; Index < ArraySize; ++Index)
        {
            FName ActualClassName = NAME_None;
            if (Ar.IsSaving())
            {
                if constexpr (bIsUObject)
                {
                    ActualClassName = Array.ContainerPrivate[Index]->GetClass()->GetFName();
                }
            }
            
            Ar << ActualClassName;
            
            if (Ar.IsLoading())
            {
                if constexpr (bIsUObject)
                {
                    UClass* ActualClass = UClass::FindClass(ActualClassName);
                    Array.ContainerPrivate[Index] = Cast<PointerToType>(FObjectFactory::ConstructObject(ActualClass, Outer));
                }
                else
                {
                    // TODO: UObject를 상속하지 않는 타입은 어떻게 처리해야할지 생각해봐야 함.
                    Array.ContainerPrivate[Index] = new PointerToType();
                }
            }
            Array.ContainerPrivate[Index]->SerializeAsset(Ar);
        }
    }
};
