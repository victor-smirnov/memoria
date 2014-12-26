
// Copyright Victor Smirnov 2013-2014.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_DISPATCHER_HPP_
#define MEMORIA_CORE_PACKED_DISPATCHER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>
#include <memoria/core/types/list/sublist.hpp>

#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/tuple.hpp>
#include <memoria/core/types/list/misc.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_rtn_type_list.hpp>



#include <utility>
#include <type_traits>

namespace memoria {

template <typename T>
struct RtnFnBase {
    typedef T ReturnType;
};


template <typename List, Int StartIdx = 0, Int ListOffsetIdx = 0> class PackedDispatcher;

template <typename Struct, Int Index>
struct StreamDescr {
    using Type = Struct;
};



template <typename Head, typename... Tail, Int Index, Int StartIdx, Int ListOffsetIdx>
class PackedDispatcher<TypeList<StreamDescr<Head, Index>, Tail...>, StartIdx, ListOffsetIdx> {
public:

    using MyType = PackedDispatcher<TypeList<StreamDescr<Head, Index>, Tail...>, StartIdx, ListOffsetIdx>;

    static const Int AllocatorIdx   = Index + StartIdx;
    static const Int ListIdx        = Index - ListOffsetIdx;

    using List              = TypeList<StreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedDispatcher<TypeList<Tail...>, StartIdx, ListOffsetIdx>;

    template<typename, Int, Int> friend class PackedDispatcher;

    template <typename T, Int Idx, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

    template <typename Fn, Int Idx, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, Idx, T...>>::RtnType;

    template<Int StreamIdx>
    using StreamTypeT = typename SelectByIndex<StreamIdx, List>::Type;

    template <typename Fn, typename... Args>
    using RtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeList<List, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeListConst<List, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using HasVoid = pd::ContainsVoidRtnType<List, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using HasVoidConst = pd::ContainsVoidRtnTypeConst<List, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using ProcessAllRtnType = typename IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        RtnTuple<Fn, Args...>
    >::Result;

    template <typename Fn, typename... Args>
    using ProcessAllRtnConstType = typename IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        ConstRtnTuple<Fn, Args...>
    >::Result;


    template <Int From = 0, Int To = sizeof...(Tail) + 1>
    using SubDispatcher = PackedDispatcher<
                                typename ::memoria::Sublist<
                                    List,
                                    From, To
                                >::Type,
                                StartIdx,
                                From
                          >;



    template <typename Fn, typename... Args>
    RtnType<Fn, ListIdx, Head*, Args...>
    dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            return NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static RtnType<Fn, ListIdx, const Head*, Args...>
    dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            return NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static RtnType<Fn, StreamIdx, StreamTypeT<StreamIdx>*, Args...>
    dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        return fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static RtnType<Fn, StreamIdx, const StreamTypeT<StreamIdx>*, Args...>
    dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        return fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchAllStatic(Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        NextDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }

        NextDispatcher::dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, args...);
        }

        NextDispatcher::dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }

        NextDispatcher::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }

        NextDispatcher::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchAll2(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAll2(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchAll2(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchAll2(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchAll2(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }




    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchStatic(Int idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            NextDispatcher::dispatchStatic(idx, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchStatic(Int idx, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchSelected(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchSelected(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};





template <typename Head, Int Index, Int StartIdx, Int ListOffsetIdx>
class PackedDispatcher<TypeList<StreamDescr<Head, Index>>, StartIdx, ListOffsetIdx> {
public:

    static const Int AllocatorIdx   = Index + StartIdx;
    static const Int ListIdx        = Index - ListOffsetIdx;


    typedef TypeList<StreamDescr<Head, Index>> List;

    template<typename, Int, Int> friend class PackedDispatcher;

    template <typename T, Int Idx, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

    template <typename Fn, Int Idx, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, Idx, T...>>::RtnType;

    template<Int StreamIdx>
    using StreamTypeT = typename SelectByIndex<StreamIdx, List>::Type;


    template <typename Fn, typename... Args>
    using RtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeList<List, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeListConst<List, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using HasVoid = pd::ContainsVoidRtnType<List, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using HasVoidConst = pd::ContainsVoidRtnTypeConst<List, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using ProcessAllRtnType = typename IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        RtnTuple<Fn, Args...>
    >::Result;

    template <typename Fn, typename... Args>
    using ProcessAllRtnConstType = typename IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        ConstRtnTuple<Fn, Args...>
    >::Result;


    template <Int From = 0, Int To = 1>
    using SubDispatcher = PackedDispatcher<
                                typename ::memoria::Sublist<
                                    TypeList<
                                        StreamDescr<Head, Index>
                                    >,
                                    From, To
                                >::Type,
                                StartIdx,
                                From
                          >;


    template <typename Fn, typename... Args>
    static RtnType<Fn, ListIdx, Head*, Args...>
    dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }


    template <typename Fn, typename... Args>
    static RtnType<Fn, ListIdx, const Head*, Args...>
    dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static RtnType<Fn, StreamIdx, StreamTypeT<StreamIdx>*, Args...>
    dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        return fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static RtnType<Fn, StreamIdx, const StreamTypeT<StreamIdx>*, Args...>
    dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        return fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchAllStatic(Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchStatic(Int idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            return fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchStatic(Int idx, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }




    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            RtnTuple<Fn, Args...>
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            RtnTuple<Fn, Args...>
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            void
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoidConst<Fn, Args...>::Value,
            void
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }






    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchSelected(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchSelected(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }
};





template <Int StartIdx, Int ListOffsetIdx>
class PackedDispatcher<TypeList<>, StartIdx, ListOffsetIdx> {
public:
    template<typename, Int, Int> friend class PackedDispatcher;

    template <typename Fn, typename... Args>
    static void dispatchAllStatic(Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt, PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt, const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}


    template <typename Fn, typename... Args>
    static void dispatchNotEmptySelected(const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchNotEmptySelected(PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    static void dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}
};




template <typename List, Int Idx = 0> struct PackedDispatchersListBuilder;

template <typename Head, typename... Tail, Int Idx>
struct PackedDispatchersListBuilder<TypeList<Head, Tail...>, Idx> {
     using Type = MergeLists<
                StreamDescr<Head, Idx>,
                typename PackedDispatchersListBuilder<
                    TypeList<Tail...>,
                    Idx + 1
                >::Type
     >;
};

template <Int Idx>
struct PackedDispatchersListBuilder<TypeList<>, Idx> {
    using Type = TypeList<>;
};

}

#endif
