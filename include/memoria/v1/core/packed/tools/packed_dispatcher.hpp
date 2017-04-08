
// Copyright 2013 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/types/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/types/list/sublist.hpp>

#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/types/list/tuple.hpp>
#include <memoria/v1/core/types/list/misc.hpp>
#include <memoria/v1/core/types/list/map.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tools/packed_rtn_type_list.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher_detail.hpp>


#include <utility>
#include <type_traits>

namespace memoria {
namespace v1 {

template <typename T>
struct RtnFnBase {
    typedef T ReturnType;
};


template <typename List, Int GroupIdx = 0, Int ListIdx = 0> class PackedDispatcher;

template <typename Struct, Int Index>
struct SubstreamDescr {
    using Type = Struct;
    static const int Value = Index;
};



template <typename Head, typename... Tail, Int Index, Int GroupIdx, Int ListIdx>
class PackedDispatcher<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx> {
public:

    using MyType = PackedDispatcher<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx>;

    static const Int AllocatorIdx = Index;

    using List              = TypeList<SubstreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedDispatcher<TypeList<Tail...>, GroupIdx, ListIdx + 1>;

    static const Int Size = ListSize<List>::Value;

    static const Int AllocatorIdxStart  = Index;
    static const Int AllocatorIdxEnd    = Select<Size - 1, List>::Value + 1;


    template<typename, Int, Int> friend class PackedDispatcher;

    template<Int StreamIdx>
    using StreamTypeT = SelectByIndex<StreamIdx, List>;

    template <typename Fn, typename... Args>
    using RtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeList<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeListConst<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >::Type;


    template <typename Fn, typename... Args>
    using HasVoid = pd::ContainsVoidRtnType<List, GroupIdx, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using HasVoidConst = pd::ContainsVoidRtnTypeConst<List, GroupIdx, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using ProcessAllRtnType = IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        RtnTuple<Fn, Args...>
    >;

    template <typename Fn, typename... Args>
    using ProcessAllRtnConstType = IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        ConstRtnTuple<Fn, Args...>
    >;



    template <Int From = 0, Int To = sizeof...(Tail) + 1, Int GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedDispatcher<
                                typename v1::Sublist<
                                    List,
                                    From, To
                                >::Type,
                                GroupIdx_
                             >;

    template <typename Subset, Int GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedDispatcher<
                                v1::ListSubset<
                                    List,
                                    Subset
                                >,
                                GroupIdx_
                           >;

    template <Int GroupIdx_>
    using GroupDispatcher = PackedDispatcher<List, GroupIdx>;

    template <template <typename> class MapFn>
    using ForAllStructs = MapTL2<List, MapFn>;

    template <typename Fn, typename... Args>
    static auto dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
        else {
            return NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static auto dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
        else {
            return NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const Int AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const Int AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            ConstRtnTuple<Fn, Args...>
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            void
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
		memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        NextDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }

        NextDispatcher::dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }

        NextDispatcher::dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }

        NextDispatcher::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

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

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);

        NextDispatcher::dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};





template <typename Head, Int Index, Int GroupIdx, Int ListIdx>
class PackedDispatcher<TypeList<SubstreamDescr<Head, Index>>, GroupIdx, ListIdx> {
public:

    static const Int AllocatorIdx   = Index;

    typedef TypeList<SubstreamDescr<Head, Index>> List;

    static const Int AllocatorIdxStart  = Index;
    static const Int AllocatorIdxEnd    = Index + 1;

    static const Int Size = ListSize<List>::Value;

    template<typename, Int, Int> friend class PackedDispatcher;


    template<Int StreamIdx>
    using StreamTypeT = SelectByIndex<StreamIdx, List>;


    template <typename Fn, typename... Args>
    using RtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeList<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = typename MakeTupleH<
            typename pd::MakeRtnTypeListConst<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >::Type;

    template <typename Fn, typename... Args>
    using HasVoid = pd::ContainsVoidRtnType<List, GroupIdx, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using HasVoidConst = pd::ContainsVoidRtnTypeConst<List, GroupIdx, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using ProcessAllRtnType = IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        RtnTuple<Fn, Args...>
    >;

    template <typename Fn, typename... Args>
    using ProcessAllRtnConstType = IfThenElse<
        HasVoid<Fn, Args...>::Value,
        void,
        ConstRtnTuple<Fn, Args...>
    >;


    template <Int From = 0, Int To = 1, Int GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedDispatcher<
                                typename v1::Sublist<
                                    TypeList<SubstreamDescr<Head, Index>>,
                                    From, To
                                >::Type,
                                GroupIdx_
                          >;

    template <typename Subset, Int GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedDispatcher<
                                    v1::ListSubset<
                                        TypeList<SubstreamDescr<Head, Index>>,
                                        Subset
                                    >,
                                    GroupIdx_
                              >;

    template <template <typename> class MapFn>
    using ForAllStructs = MapTL2<List, MapFn>;

    template <typename Fn, typename... Args>
    static auto dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }


    template <typename Fn, typename... Args>
    static auto dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
        else {
            throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
        }
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const Int AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <Int StreamIdx, typename Fn, typename... Args>
    static auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const Int AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            ConstRtnTuple<Fn, Args...>
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            void
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }




    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static void dispatchNotEmpty(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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
            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }
};





template <Int GroupIdx, Int ListOffsetIdx>
class PackedDispatcher<TypeList<>, GroupIdx, ListOffsetIdx> {
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
                SubstreamDescr<Head, Idx>,
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

}}
