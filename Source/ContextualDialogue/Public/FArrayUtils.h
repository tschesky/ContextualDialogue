#pragma once

class FArrayUtils
{
private:
    // HeapSiftDown() with custom swap function support
    template<typename ElementType, typename InAllocator, typename SwapFunctionType, typename PredicateType = TLess<>>
    FORCEINLINE static void HeapSiftDown(TArray<ElementType, InAllocator>& RESTRICT Array, const int iFirst, int Index, const int Count, const SwapFunctionType& RESTRICT SwapFunction, PredicateType Predicate)
    {
        const ElementType* RESTRICT data = Array.GetData() + iFirst;
        while (Index * 2 + 1 < Count)
        {
            const int32 LeftChildIndex = Index * 2 + 1;
            const int32 RightChildIndex = LeftChildIndex + 1;
 
            int32 MinChildIndex = LeftChildIndex;
            if (RightChildIndex < Count)
                MinChildIndex = Predicate(data[LeftChildIndex], data[RightChildIndex]) ? LeftChildIndex : RightChildIndex;
 
            if (!Predicate(data[MinChildIndex], data[Index]))
                break;
 
            SwapFunction(iFirst + Index, iFirst + MinChildIndex);
            Index = MinChildIndex;
        }
    }
 
    // HeapifyInternal() with custom swap function support
    template<typename ElementType, typename InAllocator, typename SwapFunctionType, typename PredicateType = TLess<>>
    FORCEINLINE static void HeapifyInternal(TArray<ElementType, InAllocator>& RESTRICT Array, const int iFirst, const int Num, const SwapFunctionType& RESTRICT SwapFunction, PredicateType Predicate)
    {
        for (int32 Index = (Num - 2) / 2; Index >= 0; Index--)
            HeapSiftDown(Array, iFirst, Index, Num, SwapFunction, Predicate);
    }
 
    // HeapSortInternal() with custom swap function support
    template<typename ElementType, typename InAllocator, typename SwapFunctionType, typename PredicateType = TLess<>>
    FORCEINLINE static void HeapSortInternal(TArray<ElementType, InAllocator>& RESTRICT Array, const int iFirst, const int Num, const SwapFunctionType& RESTRICT SwapFunction, PredicateType Predicate)
    {
        TReversePredicate<PredicateType> ReversePredicateWrapper(Predicate); // Reverse the predicate to build a max-heap instead of a min-heap
        HeapifyInternal(Array, iFirst, Num, SwapFunction, ReversePredicateWrapper);
 
        for (int32 Index = Num - 1; Index > 0; Index--)
        {
            SwapFunction(iFirst, iFirst + Index);
            HeapSiftDown(Array, iFirst, 0, Index, SwapFunction, ReversePredicateWrapper);
        }
    }
 
public:
    // IntroSortInternal() with custom swap function support
    template<typename ElementType, typename InAllocator, typename SwapFunctionType, typename PredicateType = TLess<>>
    FORCEINLINE static void SortArray(TArray<ElementType, InAllocator>& RESTRICT Array, const int iFirst, const int Num, const SwapFunctionType& RESTRICT SwapFunction, PredicateType Predicate = TLess<>())
    {
        struct FStack
        {
            int iMin;
            int iMax;
            uint32 MaxDepth;
        };
 
        if (Num < 2)
            return;
 
        FStack RecursionStack[32] = { {iFirst, iFirst + Num - 1, (uint32)(FMath::Loge((float)Num) * 2.f)} }, Current, Inner;
        for (FStack* StackTop = RecursionStack; StackTop >= RecursionStack; --StackTop) //-V625
        {
            Current = *StackTop;
 
        Loop:
            int Count = Current.iMax - Current.iMin + 1;
 
            if (Current.MaxDepth == 0)
            {
                // We're too deep into quick sort, switch to heap sort
                HeapSortInternal(Array, Current.iMin, Count, SwapFunction, Predicate);
                continue;
            }
 
            if (Count <= 8)
            {
                // Use simple bubble-sort.
                while (Current.iMax > Current.iMin)
                {
                    int Max, Item;
                    for (Max = Current.iMin, Item = Current.iMin + 1; Item <= Current.iMax; Item++)
                        if (Predicate(Array[Max], Array[Item]))
                            Max = Item;
                    SwapFunction(Max, Current.iMax--);
                }
            }
            else
            {
                // Grab middle element so sort doesn't exhibit worst-case behavior with presorted lists.
                SwapFunction(Current.iMin + (Count / 2), Current.iMin);
 
                // Divide list into two halves, one with items <=Current.Min, the other with items >Current.Max.
                Inner.iMin = Current.iMin;
                Inner.iMax = Current.iMax + 1;
                for (; ; )
                {
                    while (++Inner.iMin <= Current.iMax && !Predicate(Array[Current.iMin], Array[Inner.iMin]));
                    while (--Inner.iMax > Current.iMin && !Predicate(Array[Inner.iMax], Array[Current.iMin]));
                    if (Inner.iMin > Inner.iMax)
                        break;
                    SwapFunction(Inner.iMin, Inner.iMax);
                }
                SwapFunction(Current.iMin, Inner.iMax);
 
                --Current.MaxDepth;
 
                // Save big half and recurse with small half.
                if (Inner.iMax - 1 - Current.iMin >= Current.iMax - Inner.iMin)
                {
                    if (Current.iMin + 1 < Inner.iMax)
                    {
                        StackTop->iMin = Current.iMin;
                        StackTop->iMax = Inner.iMax - 1;
                        StackTop->MaxDepth = Current.MaxDepth;
                        StackTop++;
                    }
                    if (Current.iMax > Inner.iMin)
                    {
                        Current.iMin = Inner.iMin;
                        goto Loop;
                    }
                }
                else
                {
                    if (Current.iMax > Inner.iMin)
                    {
                        StackTop->iMin = Inner.iMin;
                        StackTop->iMax = Current.iMax;
                        StackTop->MaxDepth = Current.MaxDepth;
                        StackTop++;
                    }
                    if (Current.iMin + 1 < Inner.iMax)
                    {
                        Current.iMax = Inner.iMax - 1;
                        goto Loop;
                    }
                }
            }
        }
    }
 
    // IntroSortInternal() with custom swap function support
    template<typename ElementType, typename InAllocator, typename SwapFunctionType, typename PredicateType = TLess<>>
    FORCEINLINE static void SortArray(TArray<ElementType, InAllocator>& RESTRICT Array, const SwapFunctionType& RESTRICT SwapFunction, PredicateType Predicate = TLess<>())
    {
        SortArray(Array, 0, Array.Num(), SwapFunction, Predicate);
    }
};