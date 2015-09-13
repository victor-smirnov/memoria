
// Copyright Victor Smirnov 2012-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_TOOLS_HPP

#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/vector_tuple.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/bt_names.hpp>

#include <memoria/core/tools/idata.hpp>
#include <memoria/core/tools/tuple_dispatcher.hpp>

#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>

#include <ostream>
#include <tuple>

namespace memoria   {
namespace bt        {


template <Int Streams, Int Idx = 0>
struct ForEachStream {
	template <typename Fn, typename... Args>
	static auto process(Int stream, Fn&& fn, Args&&... args)
	{
		if (stream == Idx)
		{
			return fn.template process<Idx>(std::forward<Args>(args)...);
		}
		else {
			return ForEachStream<Streams, Idx + 1>::process(stream, std::forward<Fn>(fn), std::forward<Args>(args)...);
		}
	}
};


template <Int Idx>
struct ForEachStream<Idx, Idx> {
	template <typename Fn, typename... Args>
	static auto process(Int stream, Fn&& fn, Args&&... args)
	{
		if (stream == Idx)
		{
			return fn.template process<Idx>(std::forward<Args>(args)...);
		}
		else {
			throw vapi::Exception(MA_SRC, SBuf()<<"Requested stream "<<stream<<" not found");
		}
	}
};


template <typename T, typename LeafPath, typename... Args>
using ItrFindFwGTRtnType = decltype(std::declval<T>().template _findFwGT<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrFindFwGERtnType = decltype(std::declval<T>().template _findFwGE<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrFindBwGTRtnType = decltype(std::declval<T>().template _findBwGT<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrFindBwGERtnType = decltype(std::declval<T>().template _findBwGE<LeafPath>(std::declval<Args>()...));



template <typename T, Int Stream, typename... Args>
using ItrSkipFwRtnType = decltype(std::declval<T>().template _skipFw<Stream>(std::declval<Args>()...));

template <typename T, Int Stream, typename... Args>
using ItrSkipBwRtnType = decltype(std::declval<T>().template _skipBw<Stream>(std::declval<Args>()...));

template <typename T, Int Stream, typename... Args>
using ItrSkipRtnType = decltype(std::declval<T>().template _skip<Stream>(std::declval<Args>()...));



template <typename T, typename LeafPath, typename... Args>
using ItrSelectFwRtnType = decltype(std::declval<T>().template _selectFw<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrSelectBwRtnType = decltype(std::declval<T>().template _selectBw<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrSelectRtnType = decltype(std::declval<T>().template _select<LeafPath>(std::declval<Args>()...));


template <typename T, typename LeafPath, typename... Args>
using ItrRankFwRtnType = decltype(std::declval<T>().template _rankFw<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrRankBwRtnType = decltype(std::declval<T>().template _rankBw<LeafPath>(std::declval<Args>()...));

template <typename T, typename LeafPath, typename... Args>
using ItrRankRtnType = decltype(std::declval<T>().template _rank<LeafPath>(std::declval<Args>()...));





template <typename List> struct TypeListToTupleH;

template <typename List>
using TypeListToTuple = typename TypeListToTupleH<List>::Type;

template <typename... List>
struct TypeListToTupleH<TL<List...>> {
	using Type = std::tuple<List...>;
};



template <typename PkdStructList> struct MakeStreamEntryTL;

template <typename Head, typename... Tail>
struct MakeStreamEntryTL<TL<Head, Tail...>> {
	using Type = AppendItemToList<
			typename PkdStructInputType<Head>::Type,
			typename MakeStreamEntryTL<TL<Tail...>>::Type
	>;
};

template <>
struct MakeStreamEntryTL<TL<>> {
	using Type = TL<>;
};



template <typename T> struct AccumulatorBuilder;

template <typename PackedStruct, typename... Tail>
struct AccumulatorBuilder<TL<PackedStruct, Tail...>> {
	using Type = MergeLists<
					memoria::core::StaticVector<BigInt, StructSizeProvider<PackedStruct>::Value>,
					typename AccumulatorBuilder<TL<Tail...>>::Type
	>;
};

template <>
struct AccumulatorBuilder<TL<>> {
	using Type = TL<>;
};





template <typename Types>
class PageUpdateManager {

    typedef Ctr<Types>                                                          CtrT;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef std::tuple<NodeBaseG, void*, Int>                                   TxnRecord;

    CtrT& ctr_;

    StaticArray<TxnRecord, 4> pages_;

public:
    PageUpdateManager(CtrT& ctr): ctr_(ctr) {}

    ~PageUpdateManager() throw()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            void* backup_buffer = std::get<1>(pages_[c]);
            ctr_.allocator().freeMemory(backup_buffer);
        }
    }

    void add(NodeBaseG& node)
    {
        MEMORIA_ASSERT_TRUE(node.isSet());

        if (pages_.capacity() > 0)
        {
            Int page_size = node->page_size();

            void* backup_buffer = ctr_.allocator().allocateMemory(page_size);
            CopyByteBuffer(node.page(), backup_buffer, page_size);

            pages_.append(TxnRecord(node, backup_buffer, page_size));
        }
        else {
            throw Exception(MA_SRC, "No space left for new pages in the PageUpdateMgr");
        }
    }

    void checkpoint(NodeBaseG& node)
    {
    	for (Int c = 0; c < pages_.getSize(); c++)
    	{
    		if (std::get<0>(pages_[c])->id() == node->id())
    		{
    			void* backup_buffer = std::get<1>(pages_[c]);
    			Int page_size       = std::get<2>(pages_[c]);
    			CopyByteBuffer(node.page(), backup_buffer, page_size);

    			return;
    		}
    	}

    	throw Exception(MA_SRC, "Unregistered page checkpointing attempt in PageUpdateMgr");
    }

    // FIXME: unify with rollback()
    void restoreNodeState()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(pages_[c]);
            void* backup_buffer = std::get<1>(pages_[c]);
            Int page_size       = std::get<2>(pages_[c]);

            CopyByteBuffer(backup_buffer, node.page(), page_size);
        }
    }

    void rollback()
    {
        for (Int c = 0; c < pages_.getSize(); c++)
        {
            NodeBaseG& node     = std::get<0>(pages_[c]);
            void* backup_buffer = std::get<1>(pages_[c]);
            Int page_size       = std::get<2>(pages_[c]);

            CopyByteBuffer(backup_buffer, node.page(), page_size);

            ctr_.allocator().freeMemory(backup_buffer);
        }

        pages_.clear();
    }
};



template <typename T>
struct ExtendIntType {
    typedef T Type;
};

template <>
struct ExtendIntType<Short> {
    typedef BigInt Type;
};

template <>
struct ExtendIntType<Int> {
    typedef BigInt Type;
};


template <>
struct ExtendIntType<Byte> {
    typedef Int Type;
};

template <>
struct ExtendIntType<UByte> {
    typedef Int Type;
};





template <typename Iterator, typename Container>
class BTree2IteratorPrefixCache {

    using IteratorPrefix = typename Container::Types::IteratorAccumulator;
    using SizePrefix = core::StaticVector<BigInt, Container::Types::Streams>;

    using MyType = BTree2IteratorPrefixCache<Iterator, Container>;

    IteratorPrefix 	prefix_;
    IteratorPrefix 	leaf_prefix_;
    SizePrefix		size_prefix_;

public:

    void init(Iterator*) {}


    void reset() {
    	prefix_ = IteratorPrefix();
    	leaf_prefix_ = IteratorPrefix();
    	size_prefix_ = SizePrefix();
    }

    const SizePrefix& size_prefix() const
    {
        return size_prefix_;
    }

    SizePrefix& size_prefix()
    {
        return size_prefix_;
    }

    const IteratorPrefix& prefixes() const
    {
        return prefix_;
    }

    IteratorPrefix& prefixes()
    {
        return prefix_;
    }

    const IteratorPrefix& leaf_prefixes() const
    {
        return leaf_prefix_;
    }

    IteratorPrefix& leaf_prefixes()
    {
        return leaf_prefix_;
    }

    void initState() {}

    bool operator==(const MyType& other) const
    {
    	return prefix_ == other.prefix_ && leaf_prefix_ == other.leaf_prefix_ && size_prefix_ == other.size_prefix_;
    }

    bool operator!=(const MyType& other) const
	{
    	return prefix_ != other.prefix_ || leaf_prefix_ != other.leaf_prefix_ || size_prefix_ != other.size_prefix_;
    }
};

template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTree2IteratorPrefixCache<I, C>& cache)
{
    out<<"IteratorPrefixCache[";
    out<<"Branch prefixes: "<<cache.prefixes()<<", Leaf Prefixes: "<<cache.leaf_prefixes()<<", Size Prefixes: "<<cache.size_prefix();
    out<<"]";

    return out;
}



template <Int...> struct Path;

template <Int Head, Int... Tail>
struct Path<Head, Tail...> {
	template <typename T>
	static auto get(T&& tuple) ->
		typename std::tuple_element<
			Head,
			typename std::remove_reference<
				decltype(Path<Tail...>::get(std::declval<decltype(tuple)>()))
			>::type
		>::type
	{
		return std::get<Head>(Path<Tail...>::get(tuple));
	}
};

template <Int Head>
struct Path<Head> {
	template <typename T>
	static auto get(T&& tuple) -> typename std::tuple_element<Head, typename std::remove_reference<T>::type>::type {
		return std::get<Head>(tuple);
	}
};



template<Int Idx>
struct TupleEntryAccessor {
	template <typename Entry>
	static auto get(Entry&& entry) {
		return std::get<Idx>(entry);
	}
};


}
}

#endif

