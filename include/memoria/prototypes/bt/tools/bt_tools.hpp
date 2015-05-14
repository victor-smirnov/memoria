
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



template <typename DataType>
struct SingleIndexUpdateData {
    Int stream_;
    Int index_;
    DataType delta_;

    SingleIndexUpdateData(Int stream, Int index, DataType delta):
        stream_(stream), index_(index), delta_(delta)
    {}

    Int stream() const      {return stream_;}
    Int index() const       {return index_;}
    DataType delta() const  {return delta_;}
};

template <typename Iterator>
class IteratorCacheBase {
    Iterator* iter_;
public:
    IteratorCacheBase() {}

    void init(Iterator* i)
    {
        iter_ = i;
    }

    void initState() {}


    void Prepare()              {}
    void nextKey(bool end)      {}
    void prevKey(bool start)    {}

    const Iterator& iterator() const {
        return *iter_;
    }

    Iterator& iterator() {
        return *iter_;
    }

    void setup(BigInt, Int) {}
};


template <typename Iterator, typename Container>
class BTreeIteratorCache: public IteratorCacheBase<Iterator> {
    typedef IteratorCacheBase<Iterator> Base;

    BigInt key_num_;
public:

    BTreeIteratorCache(): Base(), key_num_(0) {}

    BigInt& key_num()
    {
        return key_num_;
    }

    const BigInt& key_num() const
    {
        return key_num_;
    }

    void dump() const {}
};


template <
    typename I, typename C
>
std::ostream& operator<<(std::ostream& out, const BTreeIteratorCache<I, C>& cache)
{
    out<<"BTreeIteratorCache[";
    out<<"key_num: "<<cache.key_num();
    out<<"]";

    return out;
}










template <typename Tuple, Int Index = std::tuple_size<Tuple>::value> struct TupleDispatcher;

template <typename... Types, Int Index>
struct TupleDispatcher<std::tuple<Types...>, Index> {
    typedef std::tuple<Types...> Tuple;

    static const Int SIZE = std::tuple_size<Tuple>::value;

    template <typename Fn, typename... Args>
    static void dispatch(const Tuple& tuple, Fn&& fn, Args&&... args)
    {
        fn.template operator()<SIZE - Index>(std::get<SIZE - Index>(tuple), args...);
        TupleDispatcher<Tuple, Index - 1>::dispatch(tuple, std::move(fn), args...);
    }
};

template <typename... Types>
struct TupleDispatcher<std::tuple<Types...>, 0> {
    typedef std::tuple<Types...> Tuple;

    template <typename Fn, typename... Args>
    static void dispatch(const Tuple& tuple, Fn&& fn, Args&&... args){}
};


template <typename Element, Int Size>
Element GetElement(const StaticVector<Element, Size>& v, Int idx)
{
    return v[idx];
}


template <typename Value>
struct GetElementFn {

    Value value_ = 0;

    template <Int Index, typename Element>
    void operator()(const Element& element, Int tuple_index, Int value_index)
    {
        if (Index == tuple_index)
        {
            value_ = GetElement(element, value_index);
        }
    }
};

template <typename Element, typename... Types>
Element GetValue(const std::tuple<Types...>& v, Int element_idx, Int value_idx)
{
    GetElementFn<Element> fn;
    TupleDispatcher<std::tuple<Types...>>::dispatch(v, fn, element_idx, value_idx);
    return fn.value_;
}








template <typename List> struct TupleBuilder;

template <typename... Types>
struct TupleBuilder<TypeList<Types...>> {
    typedef std::tuple<Types...>                                                Type;
};





template <typename T, Int N>
struct SameTypeListBuilder {
    using Type = MergeLists<
            T,
            typename SameTypeListBuilder<T, N - 1>::Type
    >;
};

template <typename T>
struct SameTypeListBuilder<T, 0> {
    typedef TypeList<>                                                          Type;
};




template <typename List, Int Idx = 0>
class AccumulatorListBuilder;

template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class AccumulatorListBuilder<TypeList<StructsTF, Tail...>, Idx> {

public:
    using Type = MergeLists<
            Linearize<typename StructsTF::AccumulatorPart>,
            typename AccumulatorListBuilder<
                TypeList<Tail...>,
                Idx + 1
            >::Type
    >;
};



template <Int Idx>
class AccumulatorListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          Type;
};




template <typename List, Int Idx = 0>
class IteratorPrefixListBuilder;

template <
    typename StructsTF,
    typename... Tail,
    Int Idx
>
class IteratorPrefixListBuilder<TypeList<StructsTF, Tail...>, Idx> {

public:
    using Type = MergeLists<
            typename StructsTF::IteratorPrefixPart,
            typename IteratorPrefixListBuilder<
                TypeList<Tail...>,
                Idx + 1
            >::Type
    >;
};



template <Int Idx>
class IteratorPrefixListBuilder<TypeList<>, Idx> {
public:
    typedef TypeList<>                                                          Type;
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


template <typename Node>
class LayoutManager: public INodeLayoutManager {
    const Node* node_;
public:
    LayoutManager(const Node* node): node_(node) {}

    virtual Int getNodeCapacity(const Int* sizes, Int stream)
    {
        return node_->capacity(sizes, stream);
    }
};


template <typename Dispatcher>
class StaticLayoutManager: public INodeLayoutManager {
    Int block_size_;
public:

    struct NodeFn {
        typedef Int ReturnType;

        template <typename Node>
        ReturnType treeNode(const Node*, Int block_size, const Int* sizes, Int stream)
        {
            return Node::capacity(block_size, sizes, stream, false);
        }
    };

    StaticLayoutManager(Int block_size): block_size_(block_size) {}
    virtual Int getNodeCapacity(const Int* sizes, Int stream)
    {
        return Dispatcher::dispatchStatic2Rtn(true, NodeFn(), block_size_, sizes, stream);
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



template <typename List> struct StreamSourcePtrListBuilder;

template <typename Head, typename... Tail>
struct StreamSourcePtrListBuilder<TypeList<Head, Tail...>>
{
    using Type = MergeLists<
                IDataSource<Head>*,
                typename StreamSourcePtrListBuilder<
                    TypeList<Tail...>
                >::Type
    >;
};

template <>
struct StreamSourcePtrListBuilder<TypeList<>>
{
    using Type = TypeList<>;
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

//    bool operator==(const MyType& other) const
//    {
//    	return prefix_ == other.prefix_ &&  size_prefix_ == other.size_prefix_;
//    }
//
//    bool operator!=(const MyType& other) const
//	{
//    	return prefix_ != other.prefix_ ||  size_prefix_ != other.size_prefix_;
//	}

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





}
}

#endif

