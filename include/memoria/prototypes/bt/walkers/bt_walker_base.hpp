
// Copyright Victor Smirnov 2013-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_WALKER_BASE_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/types/list/list_tree.hpp>
#include <memoria/prototypes/bt/walkers/bt_walker_tools.hpp>


#include <tuple>

namespace memoria {
namespace bt1     {

namespace detail {

template <typename Accumulator, typename IdxList> struct BranchAccumWaker1;
template <typename Accumulator, typename IdxList> struct BranchAccumWalker2;

template <
	typename Accumulator,
	Int Idx,
	Int... Tail
>
struct BranchAccumWalker2<Accumulator, IntList<Idx, Tail...>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{
		BranchAccumWalker2<Accumulator, IntList<Idx, Tail...>> w;

		node->template processStreamByIdx<Idx>(w, std::get<Idx>(accum), start, end);

		BranchAccumWalker2<Accumulator, IntList<Tail...>>::process(node, accum, start, end);
	}

	template <
		typename StreamObj,
		typename T,
		Int From,
		Int To,
		template <typename, Int, Int> class AccumItem
	>
	void stream(const StreamObj* obj, AccumItem<T, From, To>& item, Int start, Int end)
	{
		obj->template sum<From>(start, end, item);
	}

	template <
		typename StreamObj,
		typename T,
		template <typename> class AccumItem
	>
	void stream(const StreamObj* obj, AccumItem<T>& item, Int start, Int end)
	{
	}
};


template <
	typename Accumulator
>
struct BranchAccumWalker2<Accumulator, IntList<>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{}
};



template <
	typename Accumulator,
	Int Idx,
	Int... Tail
>
struct BranchAccumWaker1<Accumulator, IntList<Idx, Tail...>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{
		using ItemType = typename std::tuple_element<Idx, Accumulator>::type;
		using RangeIdxList = memoria::list_tree::MakeValueList<Int, 0, std::tuple_size<ItemType>::value>;

		BranchAccumWalker2<ItemType, RangeIdxList>::process(node, std::get<Idx>(accum), start, end);

		BranchAccumWaker1<Accumulator, IntList<Tail...>>::process(node, accum, start, end);
	}
};


template <
	typename Accumulator
>
struct BranchAccumWaker1<Accumulator, IntList<>> {

	template <typename Node>
	static void process(const Node* node, Accumulator& accum, Int start, Int end)
	{}
};



template <
	typename Accum,
	typename RangeList,
	typename LeafStructList
>
struct LeafAccumWalker {

	template <Int StreamIdx, Int Idx, typename StreamObj>
	void stream(const StreamObj* obj, Int start, Int end)
	{
		using LeafPath = typename memoria::list_tree::BuildTreePath<LeafStructList, StreamIdx>::Type;
		using IdxRange = typename Select<StreamIdx, Linearize<RangeList>>::Type;

//		AccumItem<LeafStructList, LeafPath>::value();
	}
};



}

template <typename Types, typename LeafPath_>
struct WalkerTypes: Types {
	using LeafPath 		= LeafPath_;
};

template <
    typename Types,
    typename MyType
>
class WalkerBase {
protected:
    typedef Iter<typename Types::IterTypes>                                     Iterator;
    typedef typename Types::IteratorPrefix                                      IteratorPrefix;
    typedef typename Types::IteratorAccumulator                                 IteratorAccumulator;

    typedef typename Types::CtrSizeT                                            Key;

    static const Int Streams                                                    = Types::Streams;

    using LeafPath 		= typename Types::LeafPath;

    SearchType search_type_ = SearchType::GT;

    BigInt sum_             = 0;
    BigInt target_          = 0;

    WalkDirection direction_;

    Int stream_;
    Int leaf_index_;

    IteratorPrefix prefix_;

    IteratorAccumulator accumulator_;

    bool multistream_ = false;

    bool end_ = false;

private:

    struct FindNonLeafFn {
        MyType& walker_;

        FindNonLeafFn(MyType& walker): walker_(walker) {}

        template <Int StreamIndex, typename StreamType>
        Int stream(const StreamType* stream, Int index, Int start)
        {
            return walker_.template find_non_leaf<StreamIndex>(stream, index, start);
        }
    };


    struct FindLeafFn {
        MyType& walker_;

        FindLeafFn(MyType& walker): walker_(walker) {}

        template <Int StreamIndex, typename StreamType>
        Int stream(const StreamType* stream, Int start)
        {
            return walker_.template find_leaf<StreamIndex>(stream, start);
        }
    };


    struct OtherNonLeafStreamsProc {
        MyType& walker_;

        OtherNonLeafStreamsProc(MyType& walker): walker_(walker) {}

        template <Int StreamIndex, typename StreamType>
        void stream(const StreamType* stream, Int start, Int idx)
        {
            if (stream && (StreamIndex != walker_.current_stream()))
            {
                walker_.template postProcessOtherNonLeafStreams<StreamIndex>(stream, start, idx);
            }
        }
    };

    struct OtherLeafStreamsProc {
        MyType& walker_;

        OtherLeafStreamsProc(MyType& walker): walker_(walker) {}

        template <Int StreamIndex, typename StreamType>
        void stream(const StreamType* stream)
        {
            if (stream && (StreamIndex != walker_.current_stream()))
            {
                walker_.template postProcessOtherLeafStreams<StreamIndex>(stream);
            }
        }
    };

public:

    WalkerBase(Int stream, Int leaf_index, BigInt target):
        target_(target),
        stream_(stream),
        leaf_index_(leaf_index)
    {}

    const WalkDirection& direction() const {
        return direction_;
    }

    WalkDirection& direction() {
        return direction_;
    }

    void empty(Iterator& iter)
    {
        iter.idx() = 0;
    }

    BigInt sum() const {
        return sum_;
    }

    Int current_stream() const {
        return stream_;
    }

    Int leaf_index() const
    {
        return leaf_index_;
    }

    const SearchType& search_type() const {
        return search_type_;
    }

    SearchType& search_type() {
        return search_type_;
    }

    void prepare(Iterator& iter)
    {
        prefix_ = iter.cache().prefixes();
    }

    BigInt finish(Iterator& iter, Int idx)
    {
        iter.idx() = idx;

        iter.cache().prefixes() = prefix_;

        return sum_;
    }

    const IteratorAccumulator& accumulator() const {
    	return accumulator_;
    }

    IteratorAccumulator& accumulator() {
    	return accumulator_;
    }


    MyType& self() {return *T2T<MyType*>(this);}
    const MyType& self() const {return *T2T<const MyType*>(this);}


    template <Int StreamIdx, typename StreamType>
    void postProcessNonLeafStream(const StreamType*, Int, Int) {}

    template <Int StreamIdx, typename StreamType>
    void postProcessLeafStream(const StreamType*, Int, Int) {}






    template <Int StreamIdx, typename StreamType>
    void postProcessOtherNonLeafStreams(const StreamType*, Int, Int) {}

    template <Int StreamIdx, typename StreamType>
    void postProcessOtherLeafStreams(const StreamType*, Int, Int) {}



    template <typename Node>
    void postProcessNode(const Node*, Int, Int) {}

    template <typename NodeTypes>
    Int treeNode(const bt::BranchNode<NodeTypes>* node, BigInt start)
    {
    	Int index = node->template translateLeafIndexToBranchIndex<LeafPath>(this->leaf_index());

    	using BranchPath = typename bt::BranchNode<NodeTypes>::template BuildBranchPath<LeafPath>;
        Int idx = node->template processStream<BranchPath>(FindNonLeafFn(self()), index, start);

        using ItrAccList = memoria::list_tree::MakeValueList<Int, 0, std::tuple_size<IteratorAccumulator>::value>;
        detail::BranchAccumWaker1<IteratorAccumulator, ItrAccList>::process(node, accumulator_, start, idx);

        self().postProcessNode(node, start, idx);

        if (multistream_ && Streams > 1)
        {
            OtherNonLeafStreamsProc proc(self());
            node->processAll(proc, start, idx);
        }

        return idx;
    }

    template <typename NodeTypes>
    Int treeNode(const bt::LeafNode<NodeTypes>* node, BigInt start)
    {
        Int idx = node->template processStream<LeafPath>(FindLeafFn(self()), start);

        self().postProcessNode(node, start, idx);

        if (multistream_ && Streams > 1)
        {
            if (direction_ == WalkDirection::UP)
            {
                OtherLeafStreamsProc proc(self());
                node->processAll(proc);
            }
        }

        return idx;
    }

};


}
}

#endif
