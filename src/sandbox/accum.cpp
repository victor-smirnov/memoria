
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/types/list/linearize.hpp>
#include <memoria/prototypes/bt/tools/bt_accumulators.hpp>



#include <iostream>

using namespace memoria;
using namespace memoria::bt;
using namespace std;

class T{};

template <typename T, Int Indexes_>
struct PkdStruct {
	static const Int Indexes = Indexes_;
};

using BranchStructList = TypeList<
		PkdStruct<Int, 9>,

		PkdStruct<Int, 17>,

		PkdStruct<Int, 4>,

		PkdStruct<Int, 4>
>;


using LeafStructList = TypeList<
		TL<PkdStruct<Int, 4>, PkdStruct<Int, 4>>,

		PkdStruct<Int, 17>,

		PkdStruct<Int, 4>,

		PkdStruct<Int, 4>
>;

using IdxList = TypeList<
		TL<TL<IndexRange<-1, 1>, IndexRange<1, 3>>, TL<IndexRange<0, 3>>>,
		TL<IndexRange<0, 3>, IndexRange<3, 6>, IndexRange<6, 12>, IndexRange<13, 16>>,
		TL<>,
		TL<IndexRange<0, 1>, IndexRange<1, 2>>
>;




namespace detail {

template <typename List, Int Offset> struct ShiftRangeList;

template <Int From, Int To, typename... Tail, Int Offset>
struct ShiftRangeList<TypeList<IndexRange<From, To>, Tail...>, Offset> {
	using Type = MergeLists<
			IndexRange<From + Offset, To + Offset>,
			typename ShiftRangeList<TypeList<Tail...>, Offset>::Type
	>;
};


template <Int Offset>
struct ShiftRangeList<TypeList<>, Offset> {
	using Type = TypeList<>;
};


template <typename RangeList> struct GlueRanges;

template <Int From, Int Middle, Int To, typename... Tail>
struct GlueRanges<TypeList<IndexRange<From, Middle>, IndexRange<Middle, To>, Tail...>> {
	static_assert(From >= 0, "From must be >= 0");
	static_assert(Middle > From, "To must be > From");
	static_assert(To > Middle, "To must be > From");

	using Type = typename GlueRanges<TypeList<IndexRange<From, To>, Tail...>>::Type;
};

template <Int From, Int To, typename... Tail>
struct GlueRanges<TypeList<IndexRange<From, To>, Tail...>> {
	static_assert(From >= 0, "From must be >= 0");
	static_assert(To > From, "To must be > From");

	using Type = MergeLists<
			IndexRange<From, To>,
			typename GlueRanges<TypeList<Tail...>>::Type
	>;
};

template <>
struct GlueRanges<TL<>> {
	using Type = TL<>;
};


template <Int Max, typename List>
struct CheckRangeList;

template <Int Max, typename Head, typename... Tail>
struct CheckRangeList<Max, TL<Head, Tail...>> {
	static const bool Value = CheckRangeList<Max, TL<Tail...>>::Value;
};

template <Int Max, Int From, Int To>
struct CheckRangeList<Max, TL<IndexRange<From, To>>> {
	static const bool Value = To <= Max;
};

template <Int Max>
struct CheckRangeList<Max, TL<>> {
	static const bool Value = true;
};

}



template <typename BranchStruct, typename LeafStructList, typename RangeList, Int Offset> struct RangeListBuilder;

template <
	typename BranchStruct,
	typename LeafStruct, typename... LTail,
	typename RangeList,
	Int Offset
>
struct RangeListBuilder<BranchStruct, TypeList<LeafStruct, LTail...>, RangeList, Offset> {
	using Type = MergeLists<
			typename ::detail::ShiftRangeList<
				typename ListHead<RangeList>::Type,
				Offset
			>::Type,
			typename RangeListBuilder<
				BranchStruct,
				TypeList<LTail...>,
				typename ListTail<RangeList>::Type,
				Offset + IndexesSize<LeafStruct>::Value
			>::Type
	>;
};


template <typename BranchStruct, typename LeafStruct, typename RangeList, Int Offset>
struct RangeListBuilder {
	using Type = typename ::detail::ShiftRangeList<RangeList, Offset>::Type;
};

template <
	typename BranchStruct,
	typename RangeList,
	Int Offset
>
struct RangeListBuilder<BranchStruct, TypeList<>, RangeList, Offset> {
	using Type = TypeList<>;
};



template <typename BranchStructList, typename LeafStructLists, typename RangeList, Int Offset = 1> struct BranchNodeRangeListBuilder;

template <
	typename BranchStruct, typename... BTail,
	typename LeafStruct, typename... LTail,
	typename RangeList, typename... RTail,
	Int Offset
>
struct BranchNodeRangeListBuilder<TypeList<BranchStruct, BTail...>, TypeList<LeafStruct, LTail...>, TypeList<RangeList, RTail...>, Offset>
{
	using List = typename RangeListBuilder<
			BranchStruct,
			LeafStruct,
			RangeList,
			Offset
	>::Type;

	static_assert(::detail::CheckRangeList<IndexesSize<BranchStruct>::Value, List>::Value, "Invalid RangeList");

	using Type = MergeLists<
			TL<
				typename ::detail::GlueRanges<List>::Type
			>,
			typename BranchNodeRangeListBuilder<
				TypeList<BTail...>,
				TypeList<LTail...>,
				TypeList<RTail...>,
				0
			>::Type
	>;
};


template <Int Offset>
struct BranchNodeRangeListBuilder<TypeList<>, TypeList<>, TypeList<>, Offset>
{
	using Type = TypeList<>;
};



using Type = BranchNodeRangeListBuilder<
		BranchStructList,
		LeafStructList,
		IdxList
>::Type;


int main() {
	ListPrinter<Type>::print(cout);
}


