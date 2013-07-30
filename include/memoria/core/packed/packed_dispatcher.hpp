
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_DISPATCHER_HPP_
#define MEMORIA_CORE_PACKED_DISPATCHER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>


#include <memoria/core/packed/packed_allocator.hpp>


namespace memoria {

template <typename... List> class PackedDispatcher;

template <typename List> struct PackedDispatcherTool;
template <typename... List>
struct PackedDispatcherTool<TypeList<List...>> {
	typedef PackedDispatcher<List...> Type;
};

template <typename Struct, Int Index>
struct StructDescr {
	typedef Struct Type;
};

template <typename Head, typename... Tail, Int Index>
class PackedDispatcher<StructDescr<Head, Index>, Tail...> {

public:
	template <typename Fn, typename... Args>
	static void dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			Head* head = nullptr;
			if (!alloc->is_empty(Index))
			{
				head = alloc->template get<Head>(Index);
			}
			fn.template stream<Index>(head, args...);
		}
		else {
			PackedDispatcher<Tail...>::dispatch(idx, alloc, std::forward<Fn>(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static void dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			const Head* head = nullptr;
			if (!alloc->is_empty(Index))
			{
				head = alloc->template get<Head>(Index);
			}

			fn.template stream<Index>(head, args...);
		}
		else {
			PackedDispatcher<Tail...>::dispatch(idx, alloc, std::forward<Fn>(fn), args...);
		}
	}

	template <Int StreamIdx, typename Fn, typename... Args>
	static void dispatch(PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		typedef TypeList<StructDescr<Head, Index>, Tail...> List;

		typedef typename SelectByIndexTool<StreamIdx, List>::Result::Type StreamType;

		StreamType* head = nullptr;
		if (!alloc->is_empty(StreamIdx))
		{
			head = alloc->template get<StreamType>(StreamIdx);
		}


		fn.template stream<StreamIdx>(head, args...);
	}

	template <Int StreamIdx, typename Fn, typename... Args>
	static void dispatch(const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		typedef TypeList<StructDescr<Head, Index>, Tail...> List;

		typedef typename SelectByIndexTool<StreamIdx, List>::Result::Type StreamType;

		const StreamType* head = nullptr;
		if (!alloc->is_empty(StreamIdx))
		{
			head = alloc->template get<StreamType>(StreamIdx);
		}

		fn.template stream<StreamIdx>(head, args...);
	}


	template <typename Fn, typename... Args>
	static typename std::remove_reference<Fn>::type::ResultType
	dispatchRtn(Int idx, PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			Head* head = nullptr;
			if (!alloc->is_empty(Index))
			{
				head = alloc->template get<Head>(Index);
			}

			return fn.template stream<Index>(head, args...);
		}
		else {
			return PackedDispatcher<Tail...>::dispatchRtn(idx, alloc, std::forward<Fn>(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static typename std::remove_reference<Fn>::type::ResultType
	dispatchRtn(Int idx, const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			const Head* head = nullptr;
			if (!alloc->is_empty(Index))
			{
				head = alloc->template get<Head>(Index);
			}

			return fn.template stream<Index>(head, args...);
		}
		else {
			return PackedDispatcher<Tail...>::dispatchRtn(idx, alloc, std::forward<Fn>(fn), args...);
		}
	}

	template <Int StreamIdx, typename Fn, typename... Args>
	static typename std::remove_reference<Fn>::type::ResultType
	dispatchRtn(PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		typedef TypeList<StructDescr<Head, Index>, Tail...> List;

		typedef typename SelectByIndexTool<StreamIdx, List>::Result::Type StreamType;

		StreamType* head = nullptr;
		if (!alloc->is_empty(StreamIdx))
		{
			head = alloc->template get<StreamType>(StreamIdx);
		}

		return fn.template stream<StreamIdx>(head, args...);
	}

	template <Int StreamIdx, typename Fn, typename... Args>
	static typename std::remove_reference<Fn>::type::ResultType
	dispatchRtn(const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		typedef TypeList<StructDescr<Head, Index>, Tail...> List;

		typedef typename SelectByIndexTool<StreamIdx, List>::Result::Type StreamType;

		const StreamType* head = nullptr;
		if (!alloc->is_empty(StreamIdx))
		{
			head = alloc->template get<StreamType>(StreamIdx);
		}

		return fn.template stream<StreamIdx>(head, args...);
	}


	template <typename Fn, typename... Args>
	static void dispatchStatic(Int idx, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			fn.template stream<Head>(args...);
		}
		else {
			PackedDispatcher<Tail...>::dispatchStatic(idx, std::forward<Fn>(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static void dispatchAllStatic(Fn&& fn, Args... args)
	{
		Head* head = nullptr;
		fn.template stream<Index>(head, args...);
		PackedDispatcher<Tail...>::dispatchAllStatic(std::move(fn), args...);
	}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (!alloc->is_empty(Index))
		{
			Head* head = alloc->template get<Head>(Index);
			fn.template stream<Index>(head, args...);
		}

		PackedDispatcher<Tail...>::dispatchNotEmpty(alloc, std::forward<Fn>(fn), args...);
	}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (!alloc->is_empty(Index))
		{
			const Head* head = alloc->template get<Head>(Index);
			fn.template stream<Index>(head, args...);
		}

		PackedDispatcher<Tail...>::dispatchNotEmpty(alloc, std::forward<Fn>(fn), args...);
	}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if ((streams & (1ull << Index)) && !alloc->is_empty(Index))
		{
			Head* head = alloc->template get<Head>(Index);
			fn.template stream<Index>(head, args...);
		}

		PackedDispatcher<Tail...>::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), args...);
	}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if ((streams & (1ull << Index)) && !alloc->is_empty(Index))
		{
			const Head* head = alloc->template get<Head>(Index);
			fn.template stream<Index>(head, args...);
		}

		PackedDispatcher<Tail...>::dispatchNotEmpty(streams, alloc, std::forward<Fn>(fn), args...);
	}


	template <typename Fn, typename... Args>
	static void dispatchAll(PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		Head* head = nullptr;
		if (!alloc->is_empty(Index))
		{
			head = alloc->template get<Head>(Index);
		}

		fn.template stream<Index>(head, args...);

		PackedDispatcher<Tail...>::dispatchAll(alloc, std::forward<Fn>(fn), args...);
	}


	template <typename Fn, typename... Args>
	static void dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		const Head* head = nullptr;
		if (!alloc->is_empty(Index))
		{
			head = alloc->template get<Head>(Index);
		}

		fn.template stream<Index>(head, args...);

		PackedDispatcher<Tail...>::dispatchAll(alloc, std::forward<Fn>(fn), args...);
	}


	template <typename Fn, typename... Args>
	static void dispatchAll(UBigInt streams, PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		Head* head = nullptr;
		if (!alloc->is_empty(Index) && (streams & (1 << Index)))
		{
			head = alloc->template get<Head>(Index);
		}

		fn.template stream<Index>(head, args...);

		PackedDispatcher<Tail...>::dispatchAll(streams, alloc, std::forward<Fn>(fn), args...);
	}

	template <typename Fn, typename... Args>
	static void dispatchAll(UBigInt streams, const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		Head* head = nullptr;
		if (!alloc->is_empty(Index) && (streams & (1 << Index)))
		{
			head = alloc->template get<Head>(Index);
		}

		fn.template stream<Index>(head, args...);

		PackedDispatcher<Tail...>::dispatchAll(streams, alloc, std::forward<Fn>(fn), args...);
	}

	template <typename Fn, typename... Args>
	static typename std::remove_reference<Fn>::type::ResultType
	dispatchStaticRtn(Int idx, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			const Head* head = nullptr;
			return fn.template stream<Index>(head, args...);
		}
		else {
			return PackedDispatcher<Tail...>::dispatchStaticRtn(idx, std::forward<Fn>(fn), args...);
		}
	}
};

template <>
class PackedDispatcher<> {
public:
	template <typename Fn, typename... Args>
	static void dispatch(Int idx, PackedAllocator* alloc, Fn&& fn, Args...) {
		throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
	}

	template <typename Fn, typename... Args>
	static void dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args...) {
		throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
	}

	template <typename Fn, typename... Args>
	static void dispatchStatic(Int idx, Fn&& fn, Args...)
	{
		throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
	}

	template <typename Fn, typename... Args>
	static void dispatchAllStatic(Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(const PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(UBigInt, PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchNotEmpty(UBigInt, const PackedAllocator* alloc, Fn&& fn, Args...)
	{}


	template <typename Fn, typename... Args>
	static void dispatchNotEmptySelected(const PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchNotEmptySelected(PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchAll(PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static void dispatchAll(const PackedAllocator* alloc, Fn&& fn, Args...)
	{}

	template <typename Fn, typename... Args>
	static typename Fn::ResultType dispatchRtn(Int idx, PackedAllocator* alloc, Fn&& fn, Args...) {
		throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
	}

	template <typename Fn, typename... Args>
	static typename Fn::ResultType dispatchRtn(Int idx, const PackedAllocator* alloc, Fn&& fn, Args...) {
		throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
	}

	template <typename Fn, typename... Args>
	static typename Fn::ResultType dispatchStaticRtn(Int idx, Fn&& fn, Args...)
	{
		throw DispatchException(MA_SRC, SBuf()<<"Can't dispatch packed allocator structure: "<<idx);
	}
};


}


#endif
