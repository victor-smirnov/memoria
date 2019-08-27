
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/types/list/sublist.hpp>

#include <memoria/v1/core/types/fn_traits.hpp>
#include <memoria/v1/core/types/list/tuple.hpp>
#include <memoria/v1/core/types/list/misc.hpp>
#include <memoria/v1/core/types/list/transform.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator.hpp>
#include <memoria/v1/core/packed/tools/packed_rtn_type_list.hpp>
#include <memoria/v1/core/packed/tools/packed_dispatcher_detail.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>

#include <utility>
#include <type_traits>

namespace memoria {
namespace v1 {


template <typename State, typename List, int32_t GroupIdx = 0, int32_t ListIdx = 0> class PackedStatefulDispatcher;


template <typename State, typename Head, typename... Tail, int32_t Index, int32_t GroupIdx, int32_t ListIdx>
class PackedStatefulDispatcher<State, TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx> {

    const State& state_;

public:

    using MyType = PackedStatefulDispatcher<State, TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx>;

    static const int32_t AllocatorIdx = Index;

    using HeadSO = typename Head::SparseObject;

    using List              = TypeList<SubstreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedStatefulDispatcher<State, TypeList<Tail...>, GroupIdx, ListIdx + 1>;

    static const int32_t Size = ListSize<List>;

    static const int32_t AllocatorIdxStart  = Index;
    static const int32_t AllocatorIdxEnd    = Select<Size - 1, List>::Value + 1;

    template<typename, typename, int32_t, int32_t> friend class PackedStatefulDispatcher;

    template<int32_t StreamIdx>
    using StreamTypeT = Select<StreamIdx, List>;

    template <typename Fn, typename... Args>
    using RtnTuple = MakeTuple<
            typename pd::MakeRtnTypeList<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = MakeTuple<
            typename pd::MakeRtnTypeListConst<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;


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



    template <int32_t From = 0, int32_t To = sizeof...(Tail) + 1, int32_t GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedStatefulDispatcher<
                                State,
                                Sublist<
                                    List,
                                    From, To
                                >,
                                GroupIdx_
                             >;

    template <typename Subset, int32_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedStatefulDispatcher<
                                State,
                                ListSubset<
                                    List,
                                    Subset
                                >,
                                GroupIdx_
                           >;

    template <int32_t GroupIdx_>
    using GroupDispatcher = PackedStatefulDispatcher<State, List, GroupIdx>;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;

    PackedStatefulDispatcher(const State& state): state_(state) {}

    template <typename Fn, typename... Args>
    auto dispatch(int32_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            HeadSO so(&std::get<ListIdx>(state_), head);

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            return NextDispatcher(state_).dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    auto dispatch(int32_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            return NextDispatcher(state_).dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const int32_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        const typename StreamType::SparseObject so(&std::get<ListIdx>(state_), const_cast<StreamType*>(head));

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
    }


    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const int32_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        typename StreamType::SparseObject so(&std::get<ListIdx>(state_), head);

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
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

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);
        NextDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Fn, typename... Args>
    void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);

            HeadSO so(&std::get<ListIdx>(state_), head);

            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);
        }

        NextDispatcher(state_).dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        NextDispatcher(state_).dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ListIdx>(state_), head);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        NextDispatcher(state_).dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }

        NextDispatcher(state_).dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
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

        HeadSO so(&std::get<ListIdx>(state_), head);

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);

        NextDispatcher(state_).dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
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
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher(state_).dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
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

        HeadSO so(&std::get<ListIdx>(state_), head);

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);

        NextDispatcher(state_).dispatchAll2(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
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

        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);

        NextDispatcher(state_).dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
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
    dispatchStatic(int32_t idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            HeadSO so;
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);
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
    dispatchStatic(int32_t idx, Fn&& fn, Args&&... args)
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
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);

        NextDispatcher(state_).dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
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
        HeadSO so(&std::get<ListIdx>(state_), head);

        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);

        NextDispatcher(state_).dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ListIdx>(state_), head);

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher(state_).dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher(state_).dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;
        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher::dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher(state_).dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ListIdx>(state_), head);

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher(state_).dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;
        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );

        NextDispatcher::dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};





template <typename State, typename Head, int32_t Index, int32_t GroupIdx, int32_t ListIdx>
class PackedStatefulDispatcher<State, TypeList<SubstreamDescr<Head, Index>>, GroupIdx, ListIdx> {
    const State& state_;
public:

    using HeadSO = typename Head::SparseObject;

    static const int32_t AllocatorIdx   = Index;

    typedef TypeList<SubstreamDescr<Head, Index>> List;

    static const int32_t AllocatorIdxStart  = Index;
    static const int32_t AllocatorIdxEnd    = Index + 1;

    static const int32_t Size = ListSize<List>;

    template<typename, typename, int32_t, int32_t> friend class PackedStatefulDispatcher;


    template<int32_t StreamIdx>
    using StreamTypeT = Select<StreamIdx, List>;


    template <typename Fn, typename... Args>
    using RtnTuple = MakeTuple<
            typename pd::MakeRtnTypeList<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;

    template <typename Fn, typename... Args>
    using ConstRtnTuple = MakeTuple<
            typename pd::MakeRtnTypeListConst<List, GroupIdx, ListIdx, Fn, Args...>::Type
    >;

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


    template <int32_t From = 0, int32_t To = 1, int32_t GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedStatefulDispatcher<
                                State,
                                Sublist<
                                    TypeList<SubstreamDescr<Head, Index>>,
                                    From, To
                                >,
                                GroupIdx_
                          >;

    template <typename Subset, int32_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedStatefulDispatcher<
                                    State,
                                    ListSubset<
                                        TypeList<SubstreamDescr<Head, Index>>,
                                        Subset
                                    >,
                                    GroupIdx_
                              >;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;


    PackedStatefulDispatcher(const State& state): state_(state) {}


    template <typename Fn, typename... Args>
    auto dispatch(int32_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            HeadSO so(&std::get<ListIdx>(state_), head);

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            MMA1_THROW(DispatchException()) << WhatInfo(fmt::format8(u"Can't dispatch packed allocator structure: {}", idx));
        }
    }


    template <typename Fn, typename... Args>
    auto dispatch(int32_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
        else {
            MMA1_THROW(DispatchException()) << WhatInfo(fmt::format8(u"Can't dispatch packed allocator structure: {}", idx));
        }
    }


    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const int32_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        HeadSO so(&std::get<ListIdx>(state_), head);

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <int32_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const int32_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
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
        memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
            std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }




    template <typename Fn, typename... Args>
    void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ListIdx>(state_), head);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
            const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
            HeadSO so(&std::get<ListIdx>(state_), head);
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
            const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));
            memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        void
    >::type
    dispatchStatic(int32_t idx, Fn&& fn, Args&&... args)
    {
        if (idx == ListIdx)
        {
            HeadSO so;
            return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);
        }
        else {
            MMA1_THROW(DispatchException()) << WhatInfo(fmt::format8(u"Can't dispatch packed allocator structure: {}", idx));
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        ConstRtnTuple<Fn, Args...>
    >::type
    dispatchStatic(int32_t idx, Fn&& fn, Args&&... args)
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
        HeadSO so(&std::get<ListIdx>(state_), head);

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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

        HeadSO so(&std::get<ListIdx>(state_), head);

        return memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), so, std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    void dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args)
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ListIdx>(state_), head);

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;
        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
        const HeadSO so(&std::get<ListIdx>(state_), const_cast<Head*>(head));

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
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
        HeadSO so(&std::get<ListIdx>(state_), head);

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }


    template <typename Tuple, typename Fn, typename... Args>
    static void dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args)
    {
        HeadSO so;

        std::get<ListIdx>(tuple) = memoria::v1::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
        );
    }
};





template <typename State, int32_t GroupIdx, int32_t ListOffsetIdx>
class PackedStatefulDispatcher<State, TypeList<>, GroupIdx, ListOffsetIdx> {
public:
    template<typename, typename, int32_t, int32_t> friend class PackedStatefulDispatcher;

    PackedStatefulDispatcher(const State& state) {}

    template <typename Fn, typename... Args>
    static void dispatchAllStatic(Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t, PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmpty(uint64_t, const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}


    template <typename Fn, typename... Args>
    void dispatchNotEmptySelected(const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchNotEmptySelected(PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}

    template <typename Fn, typename... Args>
    void dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&...)
    {}
};




template <typename List, int32_t Idx = 0> struct PackedStatefulDispatchersListBuilder;

template <typename Head, typename... Tail, int32_t Idx>
struct PackedStatefulDispatchersListBuilder<TypeList<Head, Tail...>, Idx> {
     using Type = MergeLists<
                SubstreamDescr<Head, Idx>,
                typename PackedStatefulDispatchersListBuilder<
                    TypeList<Tail...>,
                    Idx + 1
                >::Type
     >;
};

template <int32_t Idx>
struct PackedStatefulDispatchersListBuilder<TypeList<>, Idx> {
    using Type = TypeList<>;
};

}}
