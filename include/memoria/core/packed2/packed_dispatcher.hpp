
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MEMORIA_CORE_PACKED_DISPATCHER_HPP_
#define MEMORIA_CORE_PACKED_DISPATCHER_HPP_

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>


#include <memoria/core/packed2/packed_allocator_types.hpp>


namespace memoria {

template <typename... List> class PackedDispatcher;

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
			Head* head = alloc->template get<Head>(idx);
			fn(head, args...);
		}
		else {
			PackedDispatcher<Tail...>::dispatch(idx, alloc, std::forward(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static typename Fn::ResultType dispatchRtn(Int idx, PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			Head* head = alloc->template get<Head>(idx);
			return fn(head, args...);
		}
		else {
			return PackedDispatcher<Tail...>::dispatchRtn(idx, alloc, std::forward(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static void dispatchStatic(Int idx, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			fn.template operator()<Head>(args...);
		}
		else {
			PackedDispatcher<Tail...>::dispatchStatic(idx, std::forward(fn), args...);
		}
	}



	template <typename Fn, typename... Args>
	static void dispatch(Int idx, const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			const Head* head = alloc->template get<Head>(idx);
			fn(head, args...);
		}
		else {
			PackedDispatcher<Tail...>::dispatch(idx, alloc, std::forward(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static typename Fn::ResultType dispatchRtn(Int idx, const PackedAllocator* alloc, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			const Head* head = alloc->template get<Head>(idx);
			return fn(head, args...);
		}
		else {
			return PackedDispatcher<Tail...>::dispatchRtn(idx, alloc, std::forward(fn), args...);
		}
	}

	template <typename Fn, typename... Args>
	static typename Fn::ResultType dispatchStaticRtn(Int idx, Fn&& fn, Args... args)
	{
		if (idx == Index)
		{
			return fn.template operator()<Head>(args...);
		}
		else {
			return PackedDispatcher<Tail...>::dispatchStaticRtn(idx, std::forward(fn), args...);
		}
	}
};

template <>
class PackedDispatcher<> {

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
