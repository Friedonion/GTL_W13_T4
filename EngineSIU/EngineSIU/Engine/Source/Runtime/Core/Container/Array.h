#pragma once
#include <algorithm>
#include <utility>
#include <vector>
#include <cassert>

#include "ContainerAllocator.h"
#include "Serialization/Archive.h"


struct FArrayHelper;

template <typename T, typename AllocatorType = FDefaultAllocator<T>>
class TArray
{
public:
    using SizeType = typename AllocatorType::SizeType;
    using ElementType = T;
    using ArrayType = std::vector<ElementType, AllocatorType>;

private:
    ArrayType ContainerPrivate;

public:
    // Iterator를 사용하기 위함
    auto begin() noexcept { return ContainerPrivate.begin(); }
    auto end() noexcept { return ContainerPrivate.end(); }
    auto begin() const noexcept { return ContainerPrivate.begin(); }
    auto end() const noexcept { return ContainerPrivate.end(); }
    auto rbegin() noexcept { return ContainerPrivate.rbegin(); }
    auto rend() noexcept { return ContainerPrivate.rend(); }
    auto rbegin() const noexcept { return ContainerPrivate.rbegin(); }
    auto rend() const noexcept { return ContainerPrivate.rend(); }

    T& operator[](SizeType Index);
    const T& operator[](SizeType Index) const;
    void operator+=(const TArray& OtherArray);
    TArray operator+(const TArray& OtherArray) const;

public:
    ArrayType& GetContainerPrivate() { return ContainerPrivate; }
    const ArrayType& GetContainerPrivate() const { return ContainerPrivate; }

    TArray();
    ~TArray() = default;

    // 이니셜라이저 생성자
    TArray(std::initializer_list<T> InitList);

    // 복사 생성자
    TArray(const TArray& Other);

    // 이동 생성자
    TArray(TArray&& Other) noexcept;

    // 복사 할당 연산자
    TArray& operator=(const TArray& Other);

    // 이동 할당 연산자
    TArray& operator=(TArray&& Other) noexcept;

    /** Element를 Number개 만큼 초기화 합니다. */
    void Init(const T& Element, SizeType Number);
    SizeType Add(const T& Item);
    SizeType Add(T&& Item);
    SizeType AddUnique(const T& Item);

    template <typename... Args>
    SizeType Emplace(Args&&... Item);

    /**
     * 다른 TArray의 모든 요소를 이 배열의 끝에 추가합니다.
     * @param Source 다른 TArray 객체
     */
    void Append(const TArray& Source);

    /**
     * InitList에 있는 요소들을 InIndex 위치에 삽입합니다.
     * @param InitList 삽입할 요소들의 초기화 리스트
     * @param InIndex 삽입 시작 위치의 인덱스
     * @return 삽입된 첫 번째 요소의 인덱스
     */
    SizeType Insert(std::initializer_list<ElementType> InitList, SizeType InIndex);

    /**
     * 다른 TArray의 요소들을 현재 배열의 특정 위치에 삽입합니다.
     * 
     * @param Items 삽입할 요소들이 포함된 TArray
     * @param InIndex 요소들을 삽입할 인덱스 위치 
     * @return 삽입된 첫 번째 요소의 인덱스
     */
    template <typename OtherAllocatorType>
    SizeType Insert(const TArray<ElementType, OtherAllocatorType>& Items, SizeType InIndex);

    /**
     * 하나의 요소를 특정 인덱스 위치에 삽입합니다.
     *
     * @param Item 삽입할 요소
     * @param InIndex 삽입 위치의 인덱스
     * @return 삽입된 요소의 인덱스
     */
    SizeType Insert(const ElementType& Item, SizeType InIndex);

    /**
     * 포인터가 가리키는 C-스타일 배열의 요소들을 이 배열의 끝에 추가합니다.
     * @param Ptr 추가할 요소 배열의 시작 포인터
     * @param Count 추가할 요소의 개수
     */
    void Append(const ElementType* Ptr, SizeType Count);

    /** Array가 비어있는지 확인합니다. */
    bool IsEmpty() const;

    /** Array를 비웁니다 */
    void Empty(SizeType Slack = 0);

    /** Item과 일치하는 모든 요소를 제거합니다. */
    SizeType Remove(const T& Item);

    /** 왼쪽부터 Item과 일치하는 요소를 1개 제거합니다. */
    bool RemoveSingle(const T& Item);

    /** 특정 위치에 있는 요소를 제거합니다. */
    void RemoveAt(SizeType Index);

    /** Predicate에 부합하는 모든 요소를 제거합니다. */
    template <typename Predicate>
        requires std::is_invocable_r_v<bool, Predicate, const T&>
    SizeType RemoveAll(const Predicate& Pred);

    T* GetData();
    const T* GetData() const;

    T& First();
    const T& First() const;
    T& Last(int32 IndexFromTheEnd = 0);
    const T& Last(int32 IndexFromTheEnd = 0) const;

    /**
     * Array에서 Item을 찾습니다.
     * @param Item 찾으려는 Item
     * @return Item의 인덱스, 찾을 수 없다면 -1
     */
    SizeType Find(const T& Item);
    bool Find(const T& Item, SizeType& Index);

    template <typename Predicate>
    FORCEINLINE const ElementType* FindByPredicate(Predicate Pred) const
    {
        return const_cast<TArray*>(this)->FindByPredicate(Pred);
    }

    template <typename Predicate>
    ElementType* FindByPredicate(Predicate Pred)
    {
        /*
        for (ElementType* Data = GetData(), DataEnd = Data + Num(); Data != DataEnd; ++Data)
        {
            if (std::invoke(Pred, *Data))
            {
                return Data;
            }
        }
        */
        for (int32 i = 0; i < Num(); ++i)
        {
            ElementType* Data = &ContainerPrivate[i];
            if (std::invoke(Pred, *Data))
            {
                return Data;
            }
        }

        return nullptr;
    }

    /**
     * Finds an item by predicate.
     *
     * @param Pred The predicate to match.
     * @returns Index to the first matching element, or INDEX_NONE if none is found.
     */
    template <typename Predicate>
    SizeType IndexOfByPredicate(Predicate Pred) const;

    /** 요소가 존재하는지 확인합니다. */
    bool Contains(const T& Item) const;

    /** Array Size를 가져옵니다. */
    SizeType Num() const;

    /** Array의 Capacity를 가져옵니다. */
    SizeType Max() const;

    /** Array의 Size를 Number로 설정합니다. */
    void SetNum(SizeType Number);

    /** Array의 Capacity를 Number로 설정합니다. */
    void Reserve(SizeType Number);

    /**
     * Count만큼 초기화되지 않은 공간을 확장합니다.
     * @warning std::vector의 한계로, 실제로는 AddDefaulted와 동작이 같습니다.
     */
    SizeType AddUninitialized(SizeType Count);

    /**
     * 배열 끝에 기본 생성된 요소 1개를 추가합니다.
     * @return 추가된 요소의 인덱스
     */
    SizeType AddDefaulted();

    /**
     * 배열 끝에 기본 생성된 요소를 Count개 만큼 추가합니다.
     * @param Count 추가할 요소의 개수
     * @return 추가된 첫 번째 요소의 인덱스. Count가 0 이하라면 현재 Num()을 반환할 수 있습니다.
     */
    SizeType AddDefaulted(SizeType Count);

    /** Array의 capacity를 현재 size에 맞게 축소하여 메모리를 최적화합니다. */
    void Shrink();

    void Sort();
    template <typename Compare>
        requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
    void Sort(const Compare& CompFn);

    bool IsValidIndex(uint32 ElementIndex) const;

    ElementType Pop();

    friend FArrayHelper;
};


template <typename T, typename AllocatorType>
T& TArray<T, AllocatorType>::operator[](SizeType Index)
{
    return ContainerPrivate[Index];
}

template <typename T, typename AllocatorType>
const T& TArray<T, AllocatorType>::operator[](SizeType Index) const
{
    return ContainerPrivate[Index];
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::operator+=(const TArray& OtherArray)
{
    ContainerPrivate.insert(end(), OtherArray.begin(), OtherArray.end());
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType> TArray<T, AllocatorType>::operator+(const TArray& OtherArray) const
{
    TArray Result(*this);
    Result += OtherArray;
    return Result;
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType>::TArray()
    : ContainerPrivate()
{
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType>::TArray(std::initializer_list<T> InitList)
    : ContainerPrivate(InitList)
{
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType>::TArray(const TArray& Other)
    : ContainerPrivate(Other.ContainerPrivate)
{
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType>::TArray(TArray&& Other) noexcept
    : ContainerPrivate(std::move(Other.ContainerPrivate))
{
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType>& TArray<T, AllocatorType>::operator=(const TArray& Other)
{
    if (this != &Other)
    {
        ContainerPrivate = Other.ContainerPrivate;
    }
    return *this;
}

template <typename T, typename AllocatorType>
TArray<T, AllocatorType>& TArray<T, AllocatorType>::operator=(TArray&& Other) noexcept
{
    if (this != &Other)
    {
        ContainerPrivate = std::move(Other.ContainerPrivate);
    }
    return *this;
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Init(const T& Element, SizeType Number)
{
    ContainerPrivate.assign(Number, Element);
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Add(const T& Item)
{
    return Emplace(Item);
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Add(T&& Item)
{
    return Emplace(std::move(Item));
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::AddUnique(const T& Item)
{
    if (SizeType Index; Find(Item, Index))
    {
        return Index;
    }
    return Add(Item);
}

template <typename T, typename AllocatorType>
template <typename... Args>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Emplace(Args&&... Item)
{
    ContainerPrivate.emplace_back(std::forward<Args>(Item)...);
    return Num()-1;
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Append(const TArray& Source)
{
    // 추가할 요소가 없으면 바로 반환
    if (Source.IsEmpty())
    {
        return;
    }

    // 최적화: 필요한 경우 미리 메모리를 할당하여 여러 번의 재할당 방지
    const SizeType OldSize = Num();
    const SizeType NumToAdd = Source.Num();
    const SizeType NewSize = OldSize + NumToAdd;
    if (Max() < NewSize)
    {
        Reserve(NewSize); // 필요한 만큼 (또는 그 이상) 용량 확보
    }

    // std::vector::insert를 사용하여 Source의 모든 요소를 현재 벡터의 끝(end())에 삽입
    ContainerPrivate.insert(
        ContainerPrivate.end(),          // 삽입 위치: 현재 벡터의 끝
        Source.ContainerPrivate.begin(), // 복사할 시작 이터레이터
        Source.ContainerPrivate.end()    // 복사할 끝 이터레이터
    );
}
template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Insert(std::initializer_list<ElementType> InitList, const SizeType InIndex)
{
    auto InsertPosIter = ContainerPrivate.begin() + InIndex;
    ContainerPrivate.insert(InsertPosIter, InitList);
    return InIndex;
}

template <typename T, typename AllocatorType>
template <typename OtherAllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Insert(
    const TArray<ElementType, OtherAllocatorType>& Items, const SizeType InIndex
)
{
    auto InsertPosIter = ContainerPrivate.begin() + InIndex;
    ContainerPrivate.insert(InsertPosIter, Items.begin(), Items.end());
    return InIndex;
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Insert(const ElementType& Item, SizeType InIndex)
{
    auto InsertPosIter = ContainerPrivate.begin() + InIndex;
    ContainerPrivate.insert(InsertPosIter, Item);
    return InIndex;
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Append(const ElementType* Ptr, SizeType Count)
{
    // 추가할 요소가 없거나 포인터가 유효하지 않으면 바로 반환
    if (Count <= 0)
    {
        return;
    }
    // Count가 0보다 클 때 Ptr이 nullptr이면 문제가 발생하므로 확인 (assert 또는 예외 처리 등)
    assert(Ptr != nullptr && "TArray::Append trying to append from null pointer with Count > 0");
    if (Ptr == nullptr) {
        // 실제 엔진이라면 로그를 남기거나 할 수 있음
        return;
    }


    // 최적화: 필요한 경우 미리 메모리를 할당
    const SizeType OldSize = Num();
    const SizeType NewSize = OldSize + Count;
    if (Max() < NewSize)
    {
        Reserve(NewSize);
    }

    // std::vector::insert는 포인터를 이터레이터처럼 사용할 수 있음
    ContainerPrivate.insert(
        ContainerPrivate.end(), // 삽입 위치: 현재 벡터의 끝
        Ptr,                  // 복사할 시작 포인터 (이터레이터 역할)
        Ptr + Count           // 복사할 끝 포인터 (이터레이터 역할)
    );
}

template <typename T, typename AllocatorType>
bool TArray<T, AllocatorType>::IsEmpty() const
{
    return ContainerPrivate.empty();
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Empty(SizeType Slack)
{
    ContainerPrivate.clear();

    if (Slack > 0)
    {
        // 현재 capacity가 Slack보다 작으면 늘림
        if (Max() < Slack)
        {
            ContainerPrivate.reserve(Slack);
        }
    }
    else // Slack이 0이면 가능하면 메모리 해제 시도
    {
        Shrink();
    }
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Remove(const T& Item)
{
    auto oldSize = ContainerPrivate.size();
    ContainerPrivate.erase(std::remove(ContainerPrivate.begin(), ContainerPrivate.end(), Item), ContainerPrivate.end());
    return static_cast<SizeType>(oldSize - ContainerPrivate.size());
}

template <typename T, typename AllocatorType>
bool TArray<T, AllocatorType>::RemoveSingle(const T& Item)
{
    auto it = std::find(ContainerPrivate.begin(), ContainerPrivate.end(), Item);
    if (it != ContainerPrivate.end())
    {
        ContainerPrivate.erase(it);
        return true;
    }
    return false;
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::RemoveAt(SizeType Index)
{
    if (Index >= 0 && static_cast<SizeType>(Index) < static_cast<SizeType>(ContainerPrivate.size()))
    {
        ContainerPrivate.erase(ContainerPrivate.begin() + Index);
    }
}

template <typename T, typename AllocatorType>
template <typename Predicate>
    requires std::is_invocable_r_v<bool, Predicate, const T&>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::RemoveAll(const Predicate& Pred)
{
    auto oldSize = ContainerPrivate.size();
    ContainerPrivate.erase(std::remove_if(ContainerPrivate.begin(), ContainerPrivate.end(), Pred), ContainerPrivate.end());
    return static_cast<SizeType>(oldSize - ContainerPrivate.size());
}

template <typename T, typename AllocatorType>
T* TArray<T, AllocatorType>::GetData()
{
    return ContainerPrivate.data();
}

template <typename T, typename AllocatorType>
const T* TArray<T, AllocatorType>::GetData() const
{
    return ContainerPrivate.data();
}

template <typename T, typename AllocatorType>
T& TArray<T, AllocatorType>::First()
{
    assert(!IsEmpty());
    return ContainerPrivate.front();
}

template <typename T, typename AllocatorType>
const T& TArray<T, AllocatorType>::First() const
{
    assert(!IsEmpty());
    return ContainerPrivate.front();
}

template <typename T, typename AllocatorType>
T& TArray<T, AllocatorType>::Last(int32 IndexFromTheEnd)
{
    assert(!IsEmpty());
    assert(IndexFromTheEnd >= 0 && IndexFromTheEnd < Num()); // 유효한 인덱스인지 확인
    return ContainerPrivate[Num() - 1 - IndexFromTheEnd];
}

template <typename T, typename AllocatorType>
const T& TArray<T, AllocatorType>::Last(int32 IndexFromTheEnd) const
{
    assert(!IsEmpty());
    assert(IndexFromTheEnd >= 0 && IndexFromTheEnd < Num()); // 유효한 인덱스인지 확인
    return ContainerPrivate[Num() - 1 - IndexFromTheEnd];
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Find(const T& Item)
{
    const auto it = std::find(ContainerPrivate.begin(), ContainerPrivate.end(), Item);
    return it != ContainerPrivate.end() ? std::distance(ContainerPrivate.begin(), it) : INDEX_NONE;
}

template <typename T, typename AllocatorType>
bool TArray<T, AllocatorType>::Find(const T& Item, SizeType& Index)
{
    Index = Find(Item);
    return (Index != INDEX_NONE);
}

template <typename T, typename AllocatorType>
template <typename Predicate>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::IndexOfByPredicate(Predicate Pred) const
{
    /* 기존 언리얼 코드
        const ElementType* Start = GetData();
        for (const ElementType* Data = Start, DataEnd = (Start + Num()); Data != DataEnd; ++Data)
        {
            if (std::invoke(Pred, *Data))
            {
                return static_cast<SizeType>(Data - Start);
            }
        }
        */
    for (int32 i = 0; i < Num(); ++i)
    {
        const ElementType* Data = &ContainerPrivate[i];
        if (std::invoke(Pred, *Data))
        {
            return static_cast<SizeType>(i);
        }
    }
    return INDEX_NONE;
}

template <typename T, typename AllocatorType>
bool TArray<T, AllocatorType>::Contains(const T& Item) const
{
    for (const T* Data = GetData(), *DataEnd = Data + Num(); Data != DataEnd; ++Data)
    {
        if (*Data == Item)
        {
            return true;
        }
    }
    return false;
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Num() const
{
    return ContainerPrivate.size();
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::Max() const
{
    return ContainerPrivate.capacity();
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::SetNum(SizeType Number)
{
    ContainerPrivate.resize(Number);
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Reserve(SizeType Number)
{
    ContainerPrivate.reserve(Number);
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::AddUninitialized(SizeType Count)
{
    if (Count <= 0)
    {
        return ContainerPrivate.size();
    }

    // 기존 크기 저장
    SizeType StartIndex = ContainerPrivate.size();

    // 메모리를 확장
    ContainerPrivate.resize(StartIndex + Count);

    // 새 크기를 반환
    return StartIndex;
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::AddDefaulted()
{
    // 새 요소들이 시작될 인덱스 (현재 크기)
    const SizeType StartIndex = Num();
    ContainerPrivate.emplace_back();
    return StartIndex;
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::SizeType TArray<T, AllocatorType>::AddDefaulted(SizeType Count)
{
    if (Count <= 0)
    {
        return Num();
    }

    // 새 요소들이 시작될 인덱스 (현재 크기)
    const SizeType StartIndex = Num();

    // resize를 사용하여 Count만큼 크기를 늘립니다.
    ContainerPrivate.resize(StartIndex + Count);

    // 추가된 첫 번째 요소의 인덱스 반환
    return StartIndex;
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Shrink()
{
    ContainerPrivate.shrink_to_fit();
}

template <typename T, typename AllocatorType>
void TArray<T, AllocatorType>::Sort()
{
    std::sort(ContainerPrivate.begin(), ContainerPrivate.end());
}

template <typename T, typename AllocatorType>
template <typename Compare>
    requires std::is_invocable_r_v<bool, Compare, const T&, const T&>
void TArray<T, AllocatorType>::Sort(const Compare& CompFn)
{
    std::sort(ContainerPrivate.begin(), ContainerPrivate.end(), CompFn);
}

template <typename T, typename AllocatorType>
bool TArray<T, AllocatorType>::IsValidIndex(uint32 ElementIndex) const
{
    // uint32라서 0미만은 검사 안함
    return ElementIndex < static_cast<uint32>(Num());
}

template <typename T, typename AllocatorType>
typename TArray<T, AllocatorType>::ElementType TArray<T, AllocatorType>::Pop()
{
    ElementType Element = ContainerPrivate.back();
    ContainerPrivate.pop_back();
    return Element;
}

template <typename ElementType, typename AllocatorType>
FArchive& operator<<(FArchive& Ar, TArray<ElementType, AllocatorType>& Array)
{
    using SizeType = typename TArray<ElementType, AllocatorType>::SizeType;

    // 배열 크기 직렬화
    SizeType ArraySize = Array.Num();
    Ar << ArraySize;

    if (Ar.IsLoading())
    {
        // 로드 시 배열 크기 설정
        Array.SetNum(ArraySize);
    }

    // 배열 요소 직렬화
    for (SizeType Index = 0; Index < ArraySize; ++Index)
    {
        Ar << Array[Index];
    }

    return Ar;
}

template <typename T> constexpr bool TIsTArray_V = false;

template <typename InElementType, typename InAllocatorType> constexpr bool TIsTArray_V<               TArray<InElementType, InAllocatorType>> = true;
template <typename InElementType, typename InAllocatorType> constexpr bool TIsTArray_V<const          TArray<InElementType, InAllocatorType>> = true;
template <typename InElementType, typename InAllocatorType> constexpr bool TIsTArray_V<      volatile TArray<InElementType, InAllocatorType>> = true;
template <typename InElementType, typename InAllocatorType> constexpr bool TIsTArray_V<const volatile TArray<InElementType, InAllocatorType>> = true;

template <typename T>
concept TIsTArray = TIsTArray_V<T>;
