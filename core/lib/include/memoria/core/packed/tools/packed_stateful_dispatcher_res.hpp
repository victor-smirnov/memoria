
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

#include <memoria/core/packed/tools/packed_dispatcher_res.hpp>

#include <memoria/core/tools/result.hpp>

#include <utility>
#include <type_traits>

namespace memoria {

namespace detail {

    template <size_t Idx, typename TgtTuple, typename Src>
    void disp_assign_value(TgtTuple& tuple, Result<Src>&& value) noexcept {
        std::get<Idx>(tuple) = value.get();
    }

    template <size_t Idx, typename TgtTuple>
    void disp_assign_value(TgtTuple& tuple, Result<void>&& value) noexcept {}

}

template <
        typename State,
        typename List,
        size_t AllocatorStartIdx,
        size_t GroupIdx = 0,
        size_t ListIdx = 0
>
class PackedStatefulDispatcherWithResult;


template <typename State, typename Head, typename... Tail, size_t AllocatorStartIdx, size_t Index, size_t GroupIdx, size_t ListIdx>
class PackedStatefulDispatcherWithResult<State, TypeList<SubstreamDescr<Head, Index>, Tail...>, AllocatorStartIdx, GroupIdx, ListIdx> {

    State& state_;

public:

    using MyType = PackedStatefulDispatcherWithResult<State, TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx>;

    static const size_t AllocatorIdx = Index;

    using HeadSO = typename Head::SparseObject;

    using List              = TypeList<SubstreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedStatefulDispatcherWithResult<State, TypeList<Tail...>, AllocatorStartIdx, GroupIdx, ListIdx + 1>;

    static const size_t Size = ListSize<List>;

    static const size_t AllocatorIdxStart  = Index;
    static const size_t AllocatorIdxEnd    = Select<Size - 1, List>::Value + 1;

    static const size_t ExtDataTupleIdx    = AllocatorIdx - AllocatorStartIdx;

    template<typename, typename, size_t, size_t, size_t> friend class PackedStatefulDispatcherWithResult;

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
    using SubrangeDispatcher = PackedStatefulDispatcherWithResult<
                                State,
                                Sublist<
                                    List,
                                    From, To
                                >,
                                AllocatorStartIdx,
                                GroupIdx_
                             >;

    template <typename Subset, size_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedStatefulDispatcherWithResult<
                                State,
                                ListSubset<
                                    List,
                                    Subset
                                >,
                                AllocatorStartIdx,
                                GroupIdx_
                           >;

    template <size_t GroupIdx_>
    using GroupDispatcher = PackedStatefulDispatcherWithResult<State, List, AllocatorStartIdx, GroupIdx>;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;

    PackedStatefulDispatcherWithResult(State& state) noexcept : state_(state) {}

    template <size_t SubstreamIdx>
    auto allocate_empty(PackedAllocator* alloc) noexcept ->
        Result<typename StreamTypeT<SubstreamIdx>::Type::SparseObject>
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head;
        if (alloc->is_empty(AllocatorIdx))
        {
            MEMORIA_TRY(head_res, alloc->template allocate_default<StreamType>(AllocatorIdx));
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
    auto dispatch(size_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept ->
        Result<decltype(
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn),
                HeadSO(&std::get<ExtDataTupleIdx>(state_), std::declval<Head*>()),
                std::forward<Args>(args)...
            )
        )>
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return NextDispatcher(state_).dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return NextDispatcher(state_).dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            VoidResult
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args) noexcept
    {
        HeadSO so;

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);

            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            MEMORIA_TRY_VOID(wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            }));
        }

        return NextDispatcher(state_).dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            MEMORIA_TRY_VOID(wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            }));
        }

        return NextDispatcher(state_).dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            MEMORIA_TRY_VOID(wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            }));
        }

        return NextDispatcher(state_).dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            MEMORIA_TRY_VOID(wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            }));
        }

        return NextDispatcher(state_).dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher(state_).dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher(state_).dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }



    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchAll2(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher(state_).dispatchAll2(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll2(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchAll2(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher(state_).dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchAll2(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }




    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args) noexcept
    {
        if (idx == ListIdx)
        {
            HeadSO so;

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return NextDispatcher::dispatchStatic(idx, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }

    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }



    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher(state_).dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        }));

        return NextDispatcher(state_).dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return NextDispatcher(state_).dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return NextDispatcher(state_).dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        HeadSO so;

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return NextDispatcher::dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
        MEMORIA_RETURN_IF_ERROR(element);

        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return NextDispatcher(state_).dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return NextDispatcher(state_).dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        HeadSO so;

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );

            return VoidResult::of();
        }));

        return NextDispatcher::dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};





template <typename State, typename Head, size_t AllocatorStartIdx, size_t Index, size_t GroupIdx, size_t ListIdx>
class PackedStatefulDispatcherWithResult<State, TypeList<SubstreamDescr<Head, Index>>, AllocatorStartIdx, GroupIdx, ListIdx> {
    State& state_;
public:

    using HeadSO = typename Head::SparseObject;

    static const size_t AllocatorIdx   = Index;

    typedef TypeList<SubstreamDescr<Head, Index>> List;

    static const size_t AllocatorIdxStart  = Index;
    static const size_t AllocatorIdxEnd    = Index + 1;

    static const size_t ExtDataTupleIdx    = AllocatorIdx - AllocatorStartIdx;

    static const size_t Size = ListSize<List>;

    template<typename, typename, size_t, size_t, size_t> friend class PackedStatefulDispatcherWithResult;


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
    using SubrangeDispatcher = PackedStatefulDispatcherWithResult<
                                State,
                                Sublist<
                                    TypeList<SubstreamDescr<Head, Index>>,
                                    From, To
                                >,
                                AllocatorStartIdx,
                                GroupIdx_
                          >;

    template <typename Subset, size_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedStatefulDispatcherWithResult<
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


    PackedStatefulDispatcherWithResult(State& state) noexcept: state_(state) {}


    template <size_t SubstreamIdx>
    auto allocate_empty(PackedAllocator* alloc) noexcept ->
        Result<typename StreamTypeT<SubstreamIdx>::Type::SparseObject>
    {
        using StreamDescrT  = StreamTypeT<SubstreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head;
        if (alloc->is_empty(AllocatorIdx))
        {
            MEMORIA_TRY(head_res, alloc->template allocate_default<StreamType>(AllocatorIdx));
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

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        using SubstreamSO = typename StreamType::SparseObject;

        return SubstreamSO(&std::get<AllocatorIdx - AllocatorStartIdx>(state_), const_cast<StreamType*>(head));
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept -> Result <
        decltype(
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn),
                HeadSO(&std::get<ExtDataTupleIdx>(state_), std::declval<Head*>()),
                std::forward<Args>(args)...
            )
        )
    >
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx);
        }
    }


    template <typename Fn, typename... Args>
    auto dispatch(size_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept -> Result<
        decltype(
            memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn),
                HeadSO(&std::get<ExtDataTupleIdx>(state_), std::declval<Head*>()),
                std::forward<Args>(args)...
            )
        )
    >
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx);
        }
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            VoidResult
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args) noexcept
    {
        const HeadSO so;

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }




    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return VoidResult::of();
        }
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return VoidResult::of();
        }
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return VoidResult::of();
        }
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                        std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return VoidResult::of();
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args) noexcept
    {
        if (idx == ListIdx)
        {
            HeadSO so;

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
                );
            });
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx);
        }
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchStatic(size_t idx, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }




    template <typename Fn, typename... Args>
    typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            VoidResult
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
            HasVoidConst<Fn, Args...>::Value,
            VoidResult
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }







    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(std::move(tuple));
    }


    template <typename Fn, typename... Args>
    typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
    }




    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return VoidResult::of();
    }


    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));


        return VoidResult::of();
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        HeadSO so;

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return VoidResult::of();
    }



    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        const HeadSO so(&std::get<ExtDataTupleIdx>(state_), const_cast<Head*>(head));

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });

        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return VoidResult::of();
    }


    template <typename Tuple, typename Fn, typename... Args>
    VoidResult dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }
        HeadSO so(&std::get<ExtDataTupleIdx>(state_), head);

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return VoidResult::of();
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        HeadSO so;

        auto element = wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(
                    std::forward<Fn>(fn), so, std::forward<Args>(args)...
            );
        });
        MEMORIA_RETURN_IF_ERROR(element);
        detail::disp_assign_value<ListIdx>(tuple, std::move(element));

        return VoidResult::of();
    }
};





template <typename State, size_t GroupIdx, size_t ListOffsetIdx>
class PackedStatefulDispatcherWithResult<State, TypeList<>, GroupIdx, ListOffsetIdx> {
public:
    template<typename, typename, size_t, size_t> friend class PackedStatefulDispatcherWithResult;

    PackedStatefulDispatcherWithResult(const State&) noexcept {}

    template <typename Fn, typename... Args>
    static VoidResult dispatchAllStatic(Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(uint64_t, PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmpty(uint64_t, const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmptySelected(const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchNotEmptySelected(PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchAll(PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    VoidResult dispatchAll(const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }
};

}
