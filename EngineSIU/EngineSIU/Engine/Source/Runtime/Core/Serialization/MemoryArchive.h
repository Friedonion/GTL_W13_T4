#pragma once
#include <assert.h>

#include "Archive.h"
#include "Container/Array.h"
#include "UObject/NameTypes.h"


class FMemoryArchive : public FArchive
{
protected:
    int64 Offset;

    FMemoryArchive()
        : FArchive(), Offset(0)
    {}

    virtual int64 Tell() override final
    {
        return Offset;
    }

    virtual FArchive& operator<<(FName& N) override
    {
        // Serialize the FName as a string
        if (IsLoading())
        {
            FString StringName;
            *this << StringName;
            N = FName(*StringName);
        }
        else
        {
            FString StringName = N.ToString();
            *this << StringName;
        }
        return *this;
    }

    virtual FArchive& operator<<(UObject*& Object) override
    {
        // FMemoryArchive로는 UObject 직렬화를 할 수 없음.
        assert(0);
        return *this;
    }
};

class FMemoryWriter : public FMemoryArchive
{
public:
    FMemoryWriter(TArray<uint8>& InData)
        : Data(InData)
    {
        bIsSaving = true;
        bIsLoading = false;
    }

    virtual void SaveData(const void* InData, uint64 Length) override
    {
        const int64 StartIndex = Data.Num();

        // 메모리 공간 확보
        Data.AddUninitialized(Length);

        // 데이터 복사
        FPlatformMemory::Memcpy(Data.GetData() + StartIndex, InData, Length);

        // 현재 위치 업데이트
        Offset += Length;
    }

    virtual void Seek(int64 InPos) override
    {
        Offset = InPos;
        if (Offset > Data.Num())
        {
            Data.AddUninitialized(Offset - Data.Num()); // 추가 공간 확보
        }
    }

private:
    TArray<uint8/*, FDefaultAllocator64<uint8>*/>& Data;  // TODO : Data의 DefaultAllocator를 64비트로?
};

class FMemoryReader : public FMemoryArchive
{
public:
    FMemoryReader(const TArray<uint8>& InData)
        : Data(InData)
    {
        bIsSaving = false;
        bIsLoading = true;
    }

    virtual void LoadData(void* OutData, uint64 Length) override
    {
        if (Offset + Length > Data.Num())
        {
            throw std::runtime_error("Attempted to read beyond the end of the buffer.");
        }

        // 데이터 복사
        FPlatformMemory::Memcpy(OutData, Data.GetData() + Offset, Length);

        // 현재 위치 업데이트
        Offset += Length;
    }

    virtual void Seek(int64 InPos) override
    {
        if (InPos > Data.Num())
        {
            throw std::runtime_error("Attempted to seek beyond the end of the buffer.");
        }
        Offset = InPos;
    }

private:
    const TArray<uint8>& Data;
};
