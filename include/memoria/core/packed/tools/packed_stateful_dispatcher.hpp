
// Copyright 2019 Victor Smirnov
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

#include <memoria/core/types.hpp>

#include <memoria/core/types/list/sublist.hpp>

#include <memoria/core/types/fn_traits.hpp>
#include <memoria/core/types/list/tuple.hpp>
#include <memoria/core/types/list/misc.hpp>
#include <memoria/core/types/list/transform.hpp>

#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_stateful_rtn_type_list.hpp>
#include <memoria/core/packed/tools/packed_dispatcher_detail.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/core/tools/result.hpp>

#include <utility>
#include <type_traits>

namespace memoria {

template <
        typename State,
        typename List,
        size_t AllocatorStartIdx,
        size_t GroupIdx = 0,
        size_t ListIdx = 0
>
class PackedStatefulDispatcher;


template <typename State, typename Head, typename... Tail, size_t AllocatorStartIdx, size_t Index, size_t GroupIdx, size_t ListIdx>
class PackedStatefulDispatcher<State, TypeList<SubstreamDescr<Head, Index>, Tail...>, AllocatorStartIdx, GroupIdx, ListIdx> {

    State& state_;

public:

    using MyType = PackedStatefulDispatcher<State, TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx>;

    static const size_t AllocatorIdx = Index;

    using HeadSO = typename Head::SparseObject;

    using List              = TypeList<SubstreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedStatefulDispatcher<State, TypeList<Tail...>, AllocatorStartIdx, GroupIdx, ListIdx + 1>;

    static const size_t Size = ListSize<List>;

    static const size_t AllocatorIdxStart  = Index;
    static const size_t AllocatorIdxEnd    = Select<Size - 1, List>::Value + 1;

    static const size_t ExtDataTupleIdx    = AllocatorIdx - AllocatorStartIdx;

    template<typename, typename, size_t, size_t, size_t> friend class PackedStatefulDispatcher;

    template<size_t StreamIdx>
    using StreamTypeT = Select<StreamIdx, List>;

    template <typename Fn, typename... Args>
    using RtnTuple = MakeTuple<
            typename pd_stateful::MakeRtnTypeList<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = MakeTuple<
            typename pd_stateful::MakeRtnTypeListConst<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;


    template <typename Fn, typename... Args>
    using HasVoid = pd_stateful::ContainsVoidRtnType<List, GroupIdx, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using HasVoidConst = pd_stateful::ContainsVoidRtnTypeConst<List, GroupIdx, ListIdx, Fn, Args...>;




    template <size_t From = 0, size_t To = sizeof...(Tail) + 1, size_t GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedStatefulDispatcher<
                                State,
                                Sublist<
                                    List,
                                    From, To
                                >,
                                AllocatorStartIdx,
                                GroupIdx_
                             >;

    template <typename Subset, size_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedStatefulDispatcher<
                                State,
                                ListSubset<
                                    List,
                                    Subset
                                >,
                                AllocatorStartIdx,
                                GroupIdx_
                           >;

    template <size_t GroupIdx_>
    using GroupDispatcher = PackedStatefulDispatcher<State, List, AllocatorStartIdx, GroupIdx>;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;

    PackedStatefulDispatcher(State& state) : state_(state) {}

    template <size_t SubstreamIdx>
    auto allocate_empty(PackedAllocator* alloc) ->
        Result<typename StreamTypeT<SubstreamIdx>::Type::SparseObject>
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head;
        if (alloc->is_empty(AllocatorIdx))
        {
            auto head_res = alloc->template allocate_default<StreamType>(AllocatorIdx);
            head = head_res;
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Substream {} is not empty", SubstreamIdx);
        }

        using SubstreamSO = typename StreamType::SparseObject;

        return Result<SubstreamSO>::of(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), head);
    }

    template <size_t SubstreamIdx>
    auto get(PackedAllocator* alloc)
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        using SubstreamSO = typename StreamType::SparseObject;

        return SubstreamSO(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), head);
    }

    template <size_t SubstreamIdx>
    const auto get(const PackedAllocator* alloc)
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        using SubstreamSO = typename StreamType::SparseObject;

        return SubstreamSO(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), const_cast<StreamType*>(head));
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        decltype(
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn),
                HeadSO(&std::get<ExtDataTupleIdx>(state_), std::declval<Head*>()),
                std::forward<Args>(args)...
            )
        )
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            return NextDispatcher(state_).dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            return NextDispatcher(state_).dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        const typename StreamType::SparseObject so(
                    &std::get<AllocatorIdx - AllocatorStartIdx>(state_),
                    const_cast<StreamType*>(head)
        );

        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        typename StreamType::SparseObject so(
                    &std::get<AllocatorIdx - AllocatorStartIdx>(state_), head
        );

        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
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
        HeadSO so;
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);

            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);            
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        return NextDispatcher(state_).dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        return NextDispatcher(state_).dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        return NextDispatcher(state_).dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        return NextDispatcher(state_).dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
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

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
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
    typename std::enable_if<
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

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
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
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchAll2(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAll2(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchAll2(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;
        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        return tuple;
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchAll2(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
               std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchAll2(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
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
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            HeadSO so;
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            return NextDispatcher::dispatchStatic(idx, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;
        dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
        return tuple;
    }



    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;
        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        return tuple;
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return tuple;
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher::dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));        
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher(state_).dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;        
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        return NextDispatcher::dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};





template <typename State, typename Head, size_t AllocatorStartIdx, size_t Index, size_t GroupIdx, size_t ListIdx>
class PackedStatefulDispatcher<State, TypeList<SubstreamDescr<Head, Index>>, AllocatorStartIdx, GroupIdx, ListIdx> {
    State& state_;
public:

    using HeadSO = typename Head::SparseObject;

    static const size_t AllocatorIdx   = Index;

    typedef TypeList<SubstreamDescr<Head, Index>> List;

    static const size_t AllocatorIdxStart  = Index;
    static const size_t AllocatorIdxEnd    = Index + 1;

    static const size_t ExtDataTupleIdx    = AllocatorIdx - AllocatorStartIdx;

    static const size_t Size = ListSize<List>;

    template<typename, typename, size_t, size_t, size_t> friend class PackedStatefulDispatcher;


    template<size_t StreamIdx>
    using StreamTypeT = Select<StreamIdx, List>;


    template <typename Fn, typename... Args>
    using RtnTuple = MakeTuple<
            typename pd_stateful::MakeRtnTypeList<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = MakeTuple<
            typename pd_stateful::MakeRtnTypeListConst<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;

    template <typename Fn, typename... Args>
    using HasVoid = pd_stateful::ContainsVoidRtnType<List, GroupIdx, ListIdx, Fn, Args...>;

    template <typename Fn, typename... Args>
    using HasVoidConst = pd_stateful::ContainsVoidRtnTypeConst<List, GroupIdx, ListIdx, Fn, Args...>;



    template <size_t From = 0, size_t To = 1, size_t GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedStatefulDispatcher<
                                State,
                                Sublist<
                                    TypeList<SubstreamDescr<Head, Index>>,
                                    From, To
                                >,
                                AllocatorStartIdx,
                                GroupIdx_
                          >;

    template <typename Subset, size_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedStatefulDispatcher<
                                    State,
                                    ListSubset<
                                        TypeList<SubstreamDescr<Head, Index>>,
                                        Subset
                                    >,
                                    AllocatorStartIdx,
                                    GroupIdx_
                              >;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;


    PackedStatefulDispatcher(State& state) : state_(state) {}


    template <size_t SubstreamIdx>
    auto allocate_empty(PackedAllocator* alloc) ->
        typename StreamTypeT<SubstreamIdx>::Type::SparseObject
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head;
        if (alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template allocate_default<StreamType>(AllocatorIdx);
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Substream {} is not empty", SubstreamIdx).do_throw();
        }

        using SubstreamSO = typename StreamType::SparseObject;
        return Result<SubstreamSO>::of(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), head);
    }



    template <size_t SubstreamIdx>
    auto get(PackedAllocator* alloc)
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        using SubstreamSO = typename StreamType::SparseObject;

        return SubstreamSO(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), head);
    }

    template <size_t SubstreamIdx>
    const auto get(const PackedAllocator* alloc)
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        using SubstreamSO = typename StreamType::SparseObject;
        return SubstreamSO(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), const_cast<StreamType*>(head));
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        decltype(
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn),
                HeadSO(&std::get<ExtDataTupleIdx>(state_), std::declval<Head*>()),
                std::forward<Args>(args)...
            )
        )
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx).do_throw();
        }
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args) ->
        decltype(
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn),
                HeadSO(&std::get<ExtDataTupleIdx>(state_), std::declval<Head*>()),
                std::forward<Args>(args)...
            )
        )
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx).do_throw();
        }
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        HeadSO so(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), head);
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        const HeadSO so(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), const_cast<Head*>(head));
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
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
        const HeadSO so;
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));   
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmptyVoidRes(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmptyVoidRes(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);   
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            HeadSO so;
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx).do_throw();
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;
        dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
        return tuple;
    }




    template <typename Fn, typename... Args>
    typename std::enable_if<
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
    typename std::enable_if<
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
    typename std::enable_if<
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

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }



    template <typename Fn, typename... Args>
    typename std::enable_if<
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

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }



    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        ConstRtnTuple<Fn, Args...> tuple;
        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        return tuple;
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        RtnTuple<Fn, Args...>
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        RtnTuple<Fn, Args...> tuple;
        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        return tuple;
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }




    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so; 
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }



    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);        
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;        
        std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }
};





template <typename State, size_t GroupIdx, size_t ListOffsetIdx>
class PackedStatefulDispatcher<State, TypeList<>, GroupIdx, ListOffsetIdx> {
public:
    template<typename, typename, size_t, size_t> friend class PackedStatefulDispatcher;

    PackedStatefulDispatcher(const State&) {}

    template <typename Fn, typename... Args>
    static void dispatchAllStatic(Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(PackedAllocator*, Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(const PackedAllocator*, Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t, PackedAllocator*, Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t, const PackedAllocator*, Fn&&, Args&&...)
    {}


    template <typename Fn, typename... Args>
    void dispatchNotEmptySelected(const PackedAllocator*, Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmptySelected(PackedAllocator*, Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchAll(PackedAllocator*, Fn&&, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchAll(const PackedAllocator*, Fn&&, Args&&...)
    {}
};




template <typename List, size_t Idx = 0> struct PackedStatefulDispatchersListBuilder;

template <typename Head, typename... Tail, size_t Idx>
struct PackedStatefulDispatchersListBuilder<TypeList<Head, Tail...>, Idx> {
     using Type = MergeLists<
                SubstreamDescr<Head, Idx>,
                typename PackedStatefulDispatchersListBuilder<
                    TypeList<Tail...>,
                    Idx + 1
                >::Type
     >;
};

template <size_t Idx>
struct PackedStatefulDispatchersListBuilder<TypeList<>, Idx> {
    using Type = TypeList<>;
};

}
