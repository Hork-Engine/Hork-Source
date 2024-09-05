#pragma once

#include <limits>

#ifndef UINT_MAX
#define UINT_MAX std::numeric_limits<unsigned int>::max()
#endif

#include <type_traits>

namespace _private {
struct Void {};
}

template<typename ...Args>
struct TypeList {
    typedef _private::Void Head;
    typedef _private::Void Tail;
};

typedef TypeList<> EmptyTypeList;

template<typename H, typename ...T>
struct TypeList<H, T...> {
    typedef H Head;
    typedef TypeList<T...> Tail;
};

// ==================================
// Is Empty
// ==================================
template<typename TL>
struct IsEmpty
    : std::true_type {
};

template<>
struct IsEmpty<TypeList<_private::Void, _private::Void>>
    : std::true_type {
};

template<typename ...Args>
struct IsEmpty<TypeList<Args...>>
    : std::integral_constant<bool, std::is_same<typename TypeList<Args...>::Head, _private::Void>::value&& IsEmpty<typename TypeList<Args...>::Tail>::value> {
};

// ==================================
// Contains
// ==================================
template<typename T, typename TL>
struct Contains
    : std::false_type {
};

//template<typename ...Args>
//struct Contains<_private::Void, Args...>
//        : std::false_type {
//};

template<typename T, typename ...Args>
struct Contains<T, TypeList<Args...>>
    : std::integral_constant<bool, std::is_same<typename TypeList<Args...>::Head, T>::value || Contains<T, typename TypeList<Args...>::Tail>::value> {
};

// ==================================
// Contains List
// ==================================
template<typename T, typename TL>
struct ContainsList
    : std::true_type {};

template<typename... Args>
struct ContainsList<EmptyTypeList, TypeList<Args...>> : std::true_type {};

template<typename T, typename... Args>
struct ContainsList<T, TypeList<Args...>>
    : std::integral_constant<bool, Contains<typename T::Head, TypeList<Args...>>::value && ContainsList<typename T::Tail, TypeList<Args...>>::value> {};

// ==================================
// Length
// ==================================
template<typename TL>
struct Length
    : std::integral_constant<unsigned int, 0> {
};

template<typename ...Args>
struct Length<TypeList<Args...>>
    : std::integral_constant<unsigned int, IsEmpty<TypeList<Args...>>::value ? 0 : 1 + Length<typename TypeList<Args...>::Tail>::value> {
};

// ==================================
// Type At
// ==================================
template<unsigned int N, typename TL>
struct TypeAt {
    typedef _private::Void type;
};

template<typename ...Args>
struct TypeAt<0, TypeList<Args...>> {
    typedef typename TypeList<Args...>::Head type;
};

template<unsigned int N, typename ...Args>
struct TypeAt<N, TypeList<Args...>> {
    static_assert(N < Length<TypeList<Args...>>::value, "N is too big");

    typedef typename TypeAt<N - 1, typename TypeList<Args...>::Tail>::type type;
};


// ==================================
// Append
// ==================================
template<typename TOrTL2, typename TL>
struct Append {};

template<typename T, typename ...Args>
struct Append<T, TypeList<Args...>> {
    typedef TypeList<Args..., T> type;
};

template<typename ...Args1, typename ...Args2>
struct Append<TypeList<Args1...>, TypeList<Args2...>> {
    typedef TypeList<Args2..., Args1...> type;
};


// ==================================
// Add
// ==================================
template<typename T, typename TL>
struct Add {};

template<typename T, typename ...Args>
struct Add<T, TypeList<Args...>> {
    typedef TypeList<Args..., T> type;
};


// ==================================
// Remove All
// ==================================
template<typename TOrTL2, typename TL>
struct RemoveAll {};

template<typename T, typename ...Args>
struct RemoveAll<T, TypeList<Args...>> {
private:
    typedef typename RemoveAll<T, typename TypeList<Args...>::Tail>::type Removed;
    typedef typename TypeList<Args...>::Head Head;

public:
    typedef typename std::conditional<
        std::is_same<Head, T>::value,
        Removed,
        typename Append<Removed, TypeList<Head>>::type
        >::type type;
};

template<typename T, typename Head>
struct RemoveAll<T, TypeList<Head>> {
    typedef typename std::conditional<
        std::is_same<Head, T>::value,
        EmptyTypeList,
        TypeList<Head>>::type type;
};

template<typename T>
struct RemoveAll<T, EmptyTypeList> {
    typedef EmptyTypeList type;
};


// ==================================
// Remove Duplicates
// ==================================
template<typename TL>
struct RemoveDuplicates {};

template<>
struct RemoveDuplicates<EmptyTypeList> {
    typedef EmptyTypeList type;
};

template<typename ...Args>
struct RemoveDuplicates<TypeList<Args...>> {
private:
    typedef TypeList<Args...> TL;
    typedef typename RemoveAll<typename TL::Head, typename TL::Tail>::type HeadRemovedFromTail;
    typedef typename RemoveDuplicates<HeadRemovedFromTail>::type TailWithoutDuplicates;
public:
    typedef typename Append<TailWithoutDuplicates, TypeList<typename TL::Head>>::type type;
};


// ==================================
// Extend With Const
// ==================================
template<typename TL>
struct ExtendWithConst {};

template<>
struct ExtendWithConst<EmptyTypeList> {
    typedef EmptyTypeList type;
};

template<typename ...Args>
struct ExtendWithConst<TypeList<Args...>> {
private:
    typedef TypeList<Args...> TL;

    typedef typename ExtendWithConst<typename TL::Tail>::type ExtendedTail;

    typedef typename Append<TypeList<const typename TL::Head>, ExtendedTail>::type ExtendedTailWithHead;
public:
    typedef typename Append<TypeList<typename TL::Head>, ExtendedTailWithHead>::type type;
};


// ==================================
// Find
// ==================================
namespace _private {
template<typename T, unsigned int IndexFrom, typename TL>
struct FindHelper :
    std::integral_constant<unsigned int, 0> {};

template<typename T, unsigned int IndexFrom>
struct FindHelper<T, IndexFrom, EmptyTypeList>
    : std::integral_constant<unsigned int, 0> {};

template<typename T, unsigned int IndexFrom, typename ...Args>
struct FindHelper<T, IndexFrom, TypeList<Args...>>
    : std::integral_constant<unsigned int, std::is_same<typename TypeList<Args...>::Head, T>::value ? IndexFrom : IndexFrom + 1 + FindHelper<T, IndexFrom, typename TypeList<Args...>::Tail>::value> {};
}

struct Constants {
    typedef std::integral_constant<unsigned int, UINT_MAX> npos;
};

template<typename T, typename TL>
struct Find {};

template<typename T>
struct Find<T, EmptyTypeList>
    : Constants::npos {
};

template<typename ...Args>
struct Find<_private::Void, TypeList<Args...>>
    : Constants::npos {};

template<typename T, typename ...Args>
struct Find<T, TypeList<Args...>>
    : std::integral_constant<unsigned int, Contains<T, TypeList<Args...>>::value ? _private::FindHelper<T, 0, TypeList<Args...>>::value : Constants::npos::value> {
};


// ==================================
// Slice
// ==================================
namespace _private {
template<unsigned int IndexBegin, unsigned int IndexEnd, typename TL>
struct SliceHelper {};

template<unsigned int IndexBegin, unsigned int IndexEnd>
struct SliceHelper<IndexBegin, IndexEnd, EmptyTypeList> {
    typedef EmptyTypeList type;
};

template<unsigned int IndexBegin, typename ...Args>
struct SliceHelper<IndexBegin, IndexBegin, TypeList<Args...>> {
    typedef TypeList<typename TypeAt<IndexBegin, TypeList<Args...>>::type> type;
};

template<unsigned int IndexBegin, unsigned int IndexEnd, typename ...Args>
struct SliceHelper<IndexBegin, IndexEnd, TypeList<Args...>> {
private:
    static_assert(IndexEnd >= IndexBegin, "Invalid range");
    typedef TypeList<Args...> TL;
public:
    typedef typename Add<
        typename TypeAt<IndexEnd, TL>::type,
        typename SliceHelper<IndexBegin, IndexEnd - 1, TL>::type
        >::type type;
};
}

template<unsigned int IndexBegin, unsigned int IndexAfterEnd, typename TL>
struct Slice {};

template<unsigned int IndexBegin, unsigned int IndexEnd, typename ...Args>
struct Slice<IndexBegin, IndexEnd, TypeList<Args...>> {
    typedef typename _private::SliceHelper<IndexBegin, IndexEnd, TypeList<Args...>>::type type;
};

template<unsigned int Index, typename TL>
struct CutTo {};

template<unsigned int Index, typename ...Args>
struct CutTo<Index, TypeList<Args...>> {
    typedef typename Slice<0, Index, TypeList<Args...>>::type type;
};

template<unsigned int Index, typename TL>
struct CutFrom {};

template<unsigned int Index, typename ...Args>
struct CutFrom<Index, TypeList<Args...>> {
private:
    typedef TypeList<Args...> TL;
public:
    typedef typename Slice<Index, Length<TL>::value - 1, TL>::type type;
};


// Делается по "накатанной" схеме - определяем основу шаблона.
// В этом случае шаблон инстанцируется для
// - любого другого шаблонного класса, который имеет один параметр(<b>Caller</b>)
// - любого другого типа(<b>TL</b>)
template<template<typename> class Caller, typename Arg, typename TL>
struct ForEach
{
};

// Дальше - уточняем - на самом деле мы хотим иметь только:
// - любой другой шаблонный класса, как первый аргумент(<b>Caller</b>)
// - именно какую-то версию <b>TypeList</b>, как другой аргумент
template<template<typename> class Caller, typename Arg, typename ...Args>
struct ForEach<Caller, Arg, TypeList<Args...>>
{
    typedef TypeList<Args...> TL;

    void operator()(Arg& arg) const
    {
        // Для Caller<T> - должен быть перегружен operator(),
        // где T - это TL::Head. Можно выбрать и любую другую ф-ю.
        Caller<typename TL::Head>()(arg);

        // Рекурсия - вызываем всё то же для хвоста
        ForEach<Caller, Arg, typename TL::Tail>()(arg);
    }
};

// Говорим, что когда список типов пуст - ничего не делать
template<template<typename> class Caller, typename Arg>
struct ForEach<Caller, Arg, EmptyTypeList>
{
    void operator()(Arg&) const
    {
    }
};
