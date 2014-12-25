
// Copyright Victor Smirnov 2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_BT_ACCUMULATOR_HANDLERS_HPP_
#define MEMORIA_BT_ACCUMULATOR_HANDLERS_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>

#include <tuple>

namespace memoria   {
namespace bt        {

namespace detail {

template <typename T, Int Idx> struct AccumulatorLeafItemHandler;

template <
    Int Head,
    Int... Tail,
    Int Idx
>
struct AccumulatorLeafItemHandler<IntList<Head, Tail...>, Idx>
{
    template <typename TupleItem, typename Fn, typename... Args>
    static void process(TupleItem&& tuple_item, Fn&& fn, Args&&... args)
    {
        fn.template substream<Idx, Head>(
                std::forward<TupleItem>(tuple_item),
                std::forward<Args>(args)...
        );

        AccumulatorLeafItemHandler<
            IntList<Tail...>,
            Idx + 1
        >
        ::process(
                std::forward<TupleItem>(tuple_item),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }
};


template <
    Int Head,
    Int... Tail,
    Int Idx
>
struct AccumulatorLeafItemHandler<::memoria::bt::StreamStartTag<IntList<Head, Tail...>>, Idx>
{
    template <typename TupleItem, typename Fn, typename... Args>
    static void process(TupleItem&& tuple_item, Fn&& fn, Args&&... args)
    {
        fn.template streamStart<Idx, Head>(
                std::forward<TupleItem>(tuple_item),
                std::forward<Args>(args)...
        );

        AccumulatorLeafItemHandler<
            IntList<Tail...>,
            Idx + 1
        >
        ::process(
                std::forward<TupleItem>(tuple_item),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }
};


template <
    Int Idx
>
struct AccumulatorLeafItemHandler<IntList<>, Idx>
{
    template <typename TupleItem, typename Fn, typename... Args>
    static void process(TupleItem&& tuple_item, Fn&& fn, Args&&... args){}
};


template <typename List>
struct ItemSize {
    static const Int Value = ListSize<List>::Value;
};

template <typename List>
struct ItemSize<StreamStartTag<List>> {
    static const Int Value = ListSize<List>::Value;
};


}


template <
    typename Tuple,
    typename List,
    Int From            = 0,
    Int To              = std::tuple_size<Tuple>::value,
    Int TotalIdx        = 0,
    bool Last           = To - From == 1
>
struct AccumulatorLeafHandler;

template <
    typename Tuple,
    typename List,
    Int From,
    Int To,
    Int TotalIdx
>
struct AccumulatorLeafHandler<Tuple, List, From, To, TotalIdx, false>
{
    template<typename, typename, Int, Int, Int, bool> friend class AccumulatorLeafHandlers;

    template <typename Fn, typename... Args>
    static void process(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        using ListElement = typename Select<From, List>::Result;

        detail::AccumulatorLeafItemHandler<ListElement, TotalIdx>::process(
                std::get<From>(tuple),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );

        AccumulatorLeafHandler<
            Tuple,
            List,
            From + 1,
            To,
            TotalIdx + detail::ItemSize<ListElement>::Value
        >
        ::process(
                tuple,
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }
};


template <
    typename Tuple,
    typename List,
    Int From,
    Int To,
    Int TotalIdx
>
struct AccumulatorLeafHandler<Tuple, List, From, To, TotalIdx, true>
{
    template <typename Fn, typename... Args>
    static void process(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        using ListElement = typename Select<From, List>::Result;

        detail::AccumulatorLeafItemHandler<ListElement, TotalIdx>::process(
                std::get<From>(tuple),
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }
};






template <
    typename Allocator,
    typename Dispatcher,
    template <
        Int Idx,
        Int Offset,
        bool StreamStart
    > class Fn
>
class AccumulatorHandlerFn
{
    Allocator allocator_;

public:

    template <Int Idx, typename... Args>
    using RtnFnType = auto(Args...) -> decltype(
            Dispatcher::template dispatch<Idx>(std::declval<Args>()...)
    );

    template <Int Idx, typename StreamFn, typename... T>
    using RtnType = typename FnTraits<
            RtnFnType<Idx, Allocator, StreamFn, T...>
    >::RtnType;


    AccumulatorHandlerFn(Allocator allocator):
        allocator_(allocator)
    {}

    template<Int Idx, Int Offset, typename TupleItem, typename... Args>
    auto substream(TupleItem&& item, Args&&... args)
    -> RtnType<Idx, Fn<Idx, Offset, false>, TupleItem, Args...>
    {
        return Dispatcher::template dispatch<Idx>(
                allocator_,
                Fn<Idx, Offset, false>(),
                std::forward<TupleItem>(item),
                std::forward<Args>(args)...
        );
    }

    template<Int Idx, Int Offset, typename TupleItem, typename... Args>
    auto streamStart(TupleItem&& item, Args&&... args)
    -> RtnType<Idx, Fn<Idx, Offset, true>, TupleItem, Args...>
    {
        return Dispatcher::template dispatch<Idx>(
                allocator_,
                Fn<Idx, Offset, true>(),
                std::forward<TupleItem>(item),
                std::forward<Args>(args)...
        );
    }
};



}
}



#endif
