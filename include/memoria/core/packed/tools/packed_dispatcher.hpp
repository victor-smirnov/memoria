
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

#include <memoria/core/packed/tools/packed_allocator.hpp>

#include <utility>

namespace memoria {

template <typename T>
struct RtnFnBase {
    typedef T ReturnType;
};


template <Int ListOffsetIdx, Int StartIdx, typename... List> class PackedDispatcher;

template <Int ListOffsetIdx, Int StartIdx, typename List> struct PackedDispatcherTool;

template <Int ListOffsetIdx, Int StartIdx, typename... List>
struct PackedDispatcherTool<ListOffsetIdx, StartIdx, TypeList<List...>> {
    typedef PackedDispatcher<ListOffsetIdx, StartIdx, List...> Type;
};



template <typename Struct, Int Index>
struct StreamDescr {
    typedef Struct Type;
};



template <Int ListOffsetIdx, Int StartIdx, typename Head, typename... Tail, Int Index>
class PackedDispatcher<ListOffsetIdx, StartIdx, StreamDescr<Head, Index>, Tail...> {
public:

    static const Int AllocatorIdx   = Index + StartIdx;
    static const Int ListIdx        = Index - ListOffsetIdx;


    using List              = TypeList<StreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedDispatcher<ListOffsetIdx, StartIdx, Tail...>;

    template<Int, Int, typename...> friend class PackedDispatcher;

    template <typename T, Int Idx, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

    template <typename Fn, Int Idx, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, Idx, T...>>::RtnType;

    template <typename Fn, typename... Args>
    using RtnTuple = MakeTuple<RtnType<Fn, ListIdx, Head*, Args...>, sizeof...(Tail) + 1>;


    template<Int StreamIdx>
    using StreamTypeT = typename SelectByIndexTool<StreamIdx, List>::Result::Type;

    template <Int From = 0, Int To = sizeof...(Tail) + 1>
    using SubDispatcher = typename PackedDispatcherTool<
                                From,
                                StartIdx,
                                typename ::memoria::Sublist<
                                    TypeList<
                                        StreamDescr<Head, Index>,
                                        Tail...
                                    >,
                                    From, To
                                >::Type
                          >::Type;



    template <typename Fn, typename... Args>
    static void dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    static void dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            fn.template stream<ListIdx>(head, args...);
        }
        else {
            NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    static void dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    static void dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    RtnType<Fn, ListIdx, Head*, Args...>
    dispatchRtn(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
            return NextDispatcher::dispatchRtn(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    static RtnType<Fn, ListIdx, const Head*, Args...>
    dispatchRtn(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
            return NextDispatcher::dispatchRtn(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    static auto
    dispatchRtn(PackedAllocator* alloc, Fn&& fn, Args&&... args)
        -> RtnType<Fn, StreamIdx, StreamTypeT<StreamIdx>*, Args...>
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
    static auto dispatchRtn(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
        -> RtnType<Fn, StreamIdx, const StreamTypeT<StreamIdx>*, Args...>
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
    static void dispatchStatic(Int idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            fn.template stream<Head>(std::forward<Args>(args)...);
        }
        else {
            NextDispatcher::dispatchStatic(idx, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
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
    static void dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
    static void dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
    static void dispatchAll(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static void dispatchAll(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static RtnType<Fn, ListIdx, const Head*, Args...>
    dispatchStaticRtn(Int idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            return fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            return NextDispatcher::dispatchStaticRtn(idx, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }



    template <typename Fn, typename... Args>
    static auto dispatchAllRtn(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        RtnTuple<Fn, const Head*, Args...>
    {
        RtnTuple<Fn, const Head*, Args...> tuple;

        dispatchAllRtn(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }

    template <typename Fn, typename... Args>
    static auto dispatchAllRtn(const PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        RtnTuple<Fn, const Head*, Args...>
    {
        RtnTuple<Fn, const Head*, Args...> tuple;

        dispatchAllRtn(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }

    template <typename Fn, typename... Args>
    static auto dispatchAllRtn(PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        RtnTuple<Fn, Head*, Args...>
    {
        RtnTuple<Fn, Head*, Args...> tuple;

        dispatchAllRtn(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllRtn_(Tuple& tuple, UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchRtnAll(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllRtn(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllRtn(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllRtn(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllRtn(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



};





template <Int ListOffsetIdx, Int StartIdx, typename Head, Int Index>
class PackedDispatcher<ListOffsetIdx, StartIdx, StreamDescr<Head, Index>> {
public:

    static const Int AllocatorIdx   = Index + StartIdx;
    static const Int ListIdx        = Index - ListOffsetIdx;


    typedef TypeList<StreamDescr<Head, Index>> List;

    template<Int, Int, typename...> friend class PackedDispatcher;

    template <typename T, Int Idx, typename... Args>
    using FnType = auto(Args...) -> decltype(std::declval<T>().template stream<Idx>(std::declval<Args>()...));

    template <typename Fn, Int Idx, typename... T>
    using RtnType = typename FnTraits<FnType<typename std::remove_reference<Fn>::type, Idx, T...>>::RtnType;

    template <typename Fn, typename... Args>
    using RtnTuple = MakeTuple<RtnType<Fn, ListIdx, Args...>, 1>;


    template<Int StreamIdx>
    using StreamTypeT = typename SelectByIndexTool<StreamIdx, List>::Result::Type;

    template <Int From = 0, Int To = 1>
    using SubDispatcher = typename PackedDispatcherTool<
                                From,
                                StartIdx,
                                typename ::memoria::Sublist<
                                    TypeList<
                                        StreamDescr<Head, Index>
                                    >,
                                    From, To
                                >::Type
                          >::Type;


    template <typename Fn, typename... Args>
    static void dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }

    template <typename Fn, typename... Args>
    static void dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    static void dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    static void dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        typedef StreamTypeT<StreamIdx> StreamType;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(StreamIdx + StartIdx + ListOffsetIdx))
        {
            head = alloc->template get<StreamType>(StreamIdx + StartIdx + ListOffsetIdx);
        }

        fn.template stream<StreamIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    RtnType<Fn, ListIdx, Head*, Args...>
    dispatchRtn(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
    dispatchRtn(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
    static auto
    dispatchRtn(PackedAllocator* alloc, Fn&& fn, Args&&... args)
        -> RtnType<Fn, StreamIdx, StreamTypeT<StreamIdx>*, Args...>
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
    static auto dispatchRtn(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
        -> RtnType<Fn, StreamIdx, const StreamTypeT<StreamIdx>*, Args...>
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
    static void dispatchStatic(Int idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            fn.template stream<Head>(std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
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
    static void dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchAll(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static void dispatchAll(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static RtnType<Fn, ListIdx, const Head*, Args...>
    dispatchStaticRtn(Int idx, Fn&& fn, Args&&... args)
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
    static auto dispatchAllRtn(PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        RtnTuple<Fn, const Head*, Args...>
    {
        RtnTuple<Fn, const Head*, Args...> tuple;

        dispatchAllRtn(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    static auto dispatchAllRtn(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        RtnTuple<Fn, const Head*, Args...>
    {
        RtnTuple<Fn, const Head*, Args...> tuple;

        dispatchAllRtn(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }

    template <typename Fn, typename... Args>
    static auto dispatchAllRtn(const PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        RtnTuple<Fn, const Head*, Args...>
    {
        RtnTuple<Fn, const Head*, Args...> tuple;

        dispatchAllRtn_(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllRtn(Tuple& tuple, UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }

    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllRtn(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }

    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllRtn(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = fn.template stream<ListIdx>(head, std::forward<Args>(args)...);
    }
};





template <Int ListOffsetIdx, Int StartIdx>
class PackedDispatcher<ListOffsetIdx, StartIdx> {
public:
    template<Int, Int, typename...> friend class PackedDispatcher;

//  template <typename Fn, typename... T>
//  using RtnType = typename FnTraits<decltype(&Fn::template stream<0, void*, T...>)>::RtnType;


//    template <typename Fn, typename... Args>
//    static void dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&...) {
//        throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
//    }
//
//    template <typename Fn, typename... Args>
//    static void dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&...) {
//        throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
//    }
//
//    template <typename Fn, typename... Args>
//    static void dispatchStatic(Int idx, Fn&& fn, Args&&...)
//    {
//        throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
//    }

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

//    template <typename Fn, typename... Args>
//    static typename std::remove_reference<Fn>::type::ResultType
//    dispatchRtn(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&...) {
//        throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
//    }
//
//    template <typename Fn, typename... Args>
//    static typename std::remove_reference<Fn>::type::ResultType
//    dispatchRtn(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&...) {
//        throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
//    }
//
//    template <typename Fn, typename... Args>
//    static typename std::remove_reference<Fn>::type::ResultType
//    dispatchStaticRtn(Int idx, Fn&& fn, Args&&...)
//    {
//        throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
//    }
};




template <typename List, Int Idx = 0> struct PackedDispatchersListBuilder;

template <typename Head, typename... Tail, Int Idx>
struct PackedDispatchersListBuilder<TypeList<Head, Tail...>, Idx> {
     typedef typename MergeLists<
                StreamDescr<Head, Idx>,
                typename PackedDispatchersListBuilder<
                    TypeList<Tail...>,
                    Idx + 1
                >::Type
     >::Result                                                                  Type;
};

template <Int Idx>
struct PackedDispatchersListBuilder<TypeList<>, Idx> {
    typedef TypeList<>                                                          Type;
};

}

#endif
