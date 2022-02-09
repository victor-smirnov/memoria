
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

#include<memoria/core/packed/tools/packed_dispatcher.hpp>

namespace memoria {

template <typename List, size_t GroupIdx = 0, size_t ListIdx = 0> class PackedDispatcherWithResult;


template <typename Head, typename... Tail, size_t Index, size_t GroupIdx, size_t ListIdx>
class PackedDispatcherWithResult<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx> {
public:

    using MyType = PackedDispatcherWithResult<TypeList<SubstreamDescr<Head, Index>, Tail...>, GroupIdx, ListIdx>;

    static const size_t AllocatorIdx = Index;

    using List              = TypeList<SubstreamDescr<Head, Index>, Tail...>;
    using NextDispatcher    = PackedDispatcherWithResult<TypeList<Tail...>, GroupIdx, ListIdx + 1>;

    static const size_t Size = ListSize<List>;

    static const size_t AllocatorIdxStart  = Index;
    static const size_t AllocatorIdxEnd    = Select<Size - 1, List>::Value + 1;

    template<typename, size_t, size_t> friend class PackedDispatcherWithResult;

    template<size_t StreamIdx>
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

    template <size_t From = 0, size_t To = sizeof...(Tail) + 1, size_t GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedDispatcherWithResult<
                                Sublist<
                                    List,
                                    From, To
                                >,
                                GroupIdx_
                             >;

    template <typename Subset, size_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedDispatcherWithResult<
                                ListSubset<
                                    List,
                                    Subset
                                >,
                                GroupIdx_
                           >;

    template <size_t GroupIdx_>
    using GroupDispatcher = PackedDispatcherWithResult<List, GroupIdx>;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;

    template <typename Fn, typename... Args>
    static auto dispatch(size_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx)){
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }
        else {
            return NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <typename Fn, typename... Args>
    static auto dispatch(size_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx)) {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }
        else {
            return NextDispatcher::dispatch(idx, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    static auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx)){
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    static auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx)){
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            VoidResult
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchAllStatic(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);

            MEMORIA_TRY_VOID(wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            }));
        }

        return NextDispatcher::dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);

            MEMORIA_TRY_VOID(wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            }));
        }

        return NextDispatcher::dispatchNotEmpty(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            return wrap_throwing([&](){
                memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }

        return NextDispatcher::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }

        return NextDispatcher::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchAll(alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchAll2(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx))) {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchAll2(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll2(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchAll(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchAll2(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
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
            const Head* head = nullptr;
            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        HasVoid<Fn, Args...>::Value,
        VoidResult
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx))){
            head = alloc->template get<Head>(AllocatorIdx);
        }

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchSelected(streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }




    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx))){
            head = alloc->template get<Head>(AllocatorIdx);
        }

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        }));

        return NextDispatcher::dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        }));

        return NextDispatcher::dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        }));

        return NextDispatcher::dispatchAllTupleStatic(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        }));

        return NextDispatcher::dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        }));

        return NextDispatcher::dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;

        MEMORIA_TRY_VOID(wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        }));

        return NextDispatcher::dispatchAllStaticTuple(tuple, std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
};





template <typename Head, size_t Index, size_t GroupIdx, size_t ListIdx>
class PackedDispatcherWithResult<TypeList<SubstreamDescr<Head, Index>>, GroupIdx, ListIdx> {
public:

    static const size_t AllocatorIdx   = Index;

    typedef TypeList<SubstreamDescr<Head, Index>> List;

    static const size_t AllocatorIdxStart  = Index;
    static const size_t AllocatorIdxEnd    = Index + 1;

    static const size_t Size = ListSize<List>;

    template<typename, size_t, size_t> friend class PackedDispatcherWithResult;


    template<size_t StreamIdx>
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

    template <size_t From = 0, size_t To = 1, size_t GroupIdx_ = GroupIdx>
    using SubrangeDispatcher = PackedDispatcherWithResult<
                                Sublist<
                                    TypeList<SubstreamDescr<Head, Index>>,
                                    From, To
                                >,
                                GroupIdx_
                          >;

    template <typename Subset, size_t GroupIdx_ = GroupIdx>
    using SubsetDispatcher = PackedDispatcherWithResult<
                                    ListSubset<
                                        TypeList<SubstreamDescr<Head, Index>>,
                                        Subset
                                    >,
                                    GroupIdx_
                              >;

    template <template <typename> class MapFn>
    using ForAllStructs = TransformTL<List, MapFn>;

    template <typename Fn, typename... Args>
    static auto dispatch(size_t idx, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (idx == ListIdx)
        {
            Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx);
        }
    }


    template <typename Fn, typename... Args>
    static auto dispatch(size_t idx, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept ->
        Result<decltype(memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), alloc->template get<Head>(AllocatorIdx), std::forward<Args>(args)...))>
    {
        if (idx == ListIdx)
        {
            const Head* head = nullptr;
            if (!alloc->is_empty(AllocatorIdx))
            {
                head = alloc->template get<Head>(AllocatorIdx);
            }

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }
        else {
            return MEMORIA_MAKE_GENERIC_ERROR("Can't dispatch packed allocator structure: {}", idx);
        }
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    static auto dispatch(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });
    }


    template <size_t StreamIdx, typename Fn, typename... Args>
    static auto dispatch(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        using StreamDescrT  = StreamTypeT<StreamIdx>;
        using StreamType    = typename StreamDescrT::Type;

        const size_t AllocatorIdx  = StreamDescrT::Value;

        const StreamType* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<StreamType>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, StreamIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }



    template <typename Fn, typename... Args>
    static typename std::enable_if<
            HasVoid<Fn, Args...>::Value,
            VoidResult
    >::type
    dispatchAllStatic(Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });
    }




    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }

        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if (!alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);
            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }

        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            Head* head = alloc->template get<Head>(AllocatorIdx);
            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }

        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        if ((streams & (1ull << ListIdx)) && !alloc->is_empty(AllocatorIdx))
        {
            const Head* head = alloc->template get<Head>(AllocatorIdx);

            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            });
        }

        return VoidResult::of();
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
            const Head* head = nullptr;            
            return wrap_throwing([&](){
                return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
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

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }




    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll(PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
            !HasVoid<Fn, Args...>::Value,
            Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });

        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });
    }







    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<ConstRtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        ConstRtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<ConstRtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
        !HasVoid<Fn, Args...>::Value,
        Result<RtnTuple<Fn, Args...>>
    >::type
    dispatchSelected(uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        RtnTuple<Fn, Args...> tuple;

        MEMORIA_TRY_VOID(dispatchAllTuple(tuple, streams, alloc, std::forward<Fn>(fn), std::forward<Args>(args)...));

        return Result<RtnTuple<Fn, Args...>>::of(tuple);
    }


    template <typename Fn, typename... Args>
    static typename std::enable_if<
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

        return wrap_throwing([&](){
            return memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
        });
    }




    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        });
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, uint64_t streams, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx) && (streams & (1 << ListIdx)))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        });
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTupleStatic(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;

        return wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        });
    }



    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, const PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        });
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllTuple(Tuple& tuple, PackedAllocator* alloc, Fn&& fn, Args&&... args) noexcept
    {
        Head* head = nullptr;
        if (!alloc->is_empty(AllocatorIdx))
        {
            head = alloc->template get<Head>(AllocatorIdx);
        }

        return wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        });
    }


    template <typename Tuple, typename Fn, typename... Args>
    static VoidResult dispatchAllStaticTuple(Tuple& tuple, Fn&& fn, Args&&... args) noexcept
    {
        const Head* head = nullptr;

        return wrap_throwing([&](){
            std::get<ListIdx>(tuple) = memoria::details::pd::dispatchFn<GroupIdx, AllocatorIdx, ListIdx>(std::forward<Fn>(fn), head, std::forward<Args>(args)...);
            return VoidResult::of();
        });
    }
};





template <size_t GroupIdx, size_t ListOffsetIdx>
class PackedDispatcherWithResult<TypeList<>, GroupIdx, ListOffsetIdx> {
public:
    template<typename, size_t, size_t> friend class PackedDispatcherWithResult;

    template <typename Fn, typename... Args>
    static VoidResult dispatchAllStatic(Fn&&, Args&&...) noexcept {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(uint64_t, PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmpty(uint64_t, const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }


    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmptySelected(const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchNotEmptySelected(PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchAll(PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }

    template <typename Fn, typename... Args>
    static VoidResult dispatchAll(const PackedAllocator*, Fn&&, Args&&...) noexcept
    {
        return VoidResult::of();
    }
};

}
