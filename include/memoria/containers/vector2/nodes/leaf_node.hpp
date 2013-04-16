
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINERS_VECTOR2_NODES_LEAF_HPP
#define _MEMORIA_CONTAINERS_VECTOR2_NODES_LEAF_HPP

#include <memoria/core/types/typehash.hpp>
#include <memoria/core/types/algo/select.hpp>
#include <memoria/core/tools/reflection.hpp>

#include <memoria/core/types/types.hpp>
#include <memoria/core/packed2/packed_fse_tree.hpp>
#include <memoria/core/packed2/packed_allocator.hpp>
#include <memoria/core/packed2/packed_dispatcher.hpp>

#include <memoria/prototypes/balanced_tree/baltree_tools.hpp>
#include <memoria/prototypes/balanced_tree/baltree_types.hpp>
#include <memoria/prototypes/balanced_tree/nodes/tree_node.hpp>


namespace memoria    	{
namespace mvector2 		{




template <
	typename K,
	typename V
>
struct LeafNodeTypes: PackedFSETreeTypes<K, K, V> {

};




template <
	typename Types,
	bool root, bool leaf
>
class TreeLeafNode: public balanced_tree::RootPage<typename Types::Metadata, typename Types::NodePageBase, root>
{

    static const Int  BranchingFactor                                           = PackedTreeBranchingFactor;

    typedef TreeLeafNode<Types, root, leaf>                                      Me;
    typedef TreeLeafNode<Types, root, leaf>                                      MyType;

public:
    static const UInt VERSION                                                   = 2;

    typedef balanced_tree::RootPage<
    			typename Types::Metadata,
    			typename Types::NodePageBase, root
    >  																			Base;

private:



public:

    typedef typename Types::Accumulator											Accumulator;
    typedef typename Types::Position											Position;


    typedef typename IfThenElse<
    			leaf,
    			typename Types::Value,
    			typename Types::ID
    >::Result 																	Value;
    typedef typename Types::Key                                                 Key;



    template <
        	template <typename, bool, bool> class,
        	typename,
        	bool, bool
    >
    friend class NodePageAdaptor;


	typedef LeafNodeTypes<
			Key,Value
	>																			TreeTypes;

	typedef typename PackedStructListBuilder<
	    		TreeTypes,
	    		typename Types::StreamDescriptors
	>::LeafStructList															StreamsStructList;

	typedef typename PackedDispatcherTool<StreamsStructList>::Type				Dispatcher;

private:

    PackedAllocator allocator_;

public:


    static const Int ValuesBlockIdx												= ListSize<StreamsStructList>::Value;

    TreeLeafNode(): Base() {}


private:
	struct InitFn {
		UBigInt active_streams_;

		InitFn(BigInt active_streams): active_streams_(active_streams) {}

		Int block_size(Int items_number) const
		{
			return MyType::block_size(items_number, active_streams_);
		}

		Int max_elements(Int block_size)
		{
			return block_size;
		}
	};

public:

	template <typename T>
	T* get(Int idx)
	{
		return allocator_.template get<T>(idx);
	}

	template <typename T>
	const T* get(Int idx) const
	{
		return allocator_.template get<T>(idx);
	}



private:
	struct BlockSizeFn {
		Int size_ = 0;

		template <Int StreamIndex, typename Node>
		void operator()(Node*, Int tree_size, UBigInt active_streams)
		{
			if (active_streams && (1 << StreamIndex))
			{
				size_ += Node::block_size(tree_size);
			}
		}
	};

public:
	static Int block_size(Int tree_size, UBigInt active_streams = -1)
	{
		BlockSizeFn fn;

		Dispatcher::dispatchAllStatic(fn, tree_size, active_streams);

		Int client_area = fn.size_;

		return PackedAllocator::block_size(client_area, ValuesBlockIdx + 1);
	}

	static Int max_tree_size(Int block_size, UBigInt active_streams = -1)
	{
		return FindTotalElementsNumber2(block_size, InitFn(active_streams));
	}

	static Int max_tree_size_for_block(Int block_size)
	{
		return max_tree_size(block_size - sizeof(Me) + sizeof(allocator_));
	}

	void init(Int block_size, UBigInt active_streams = -1)
	{
		init0(block_size - sizeof(Me) + sizeof(allocator_), active_streams);
	}

private:

	struct InitStructFn {
		template <Int StreamIndex, typename Tree>
		void operator()(Tree*, Int tree_size, PackedAllocator* allocator, UBigInt active_streams)
		{
			if (active_streams && (1 << StreamIndex))
			{
				Int tree_block_size = Tree::block_size(tree_size);
				allocator->template allocate<Tree>(StreamIndex, tree_block_size);
			}
		}
	};

public:

	void init0(Int block_size, UBigInt active_streams)
	{
		allocator_.init(block_size, 2);

		Int tree_size = max_tree_size(block_size, active_streams);

		Dispatcher::dispatchAllStatic(InitStructFn(), tree_size, &allocator_, active_streams);

		allocator_.template allocateArrayBySize<Value>(ValuesBlockIdx, tree_size);
	}


	void clear(const Position&, const Position&) {

	}


    void clearUnused() {}

    struct ReindexFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree)
    	{
    		tree->reindex();
    	}
    };

    void reindex()
    {
    	Dispatcher::dispatchAll(&allocator_, ReindexFn());
    }


    template <typename TreeType>
    struct TransferToFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, TreeType* other)
    	{
    		tree->transferDataTo(other->template get<Tree>(Idx));
    	}
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
    	Dispatcher::dispatchAll(&allocator_, TransferToFn<TreeType>(), other);
    }

    Int data_size() const
    {
        return sizeof(Me) + this->getDataSize();
    }



	struct CapacityFn {
		typedef Int ResultType;

		template <Int StreamIndex, typename Tree>
		ResultType operator()(const Tree* tree)
		{
			return tree->capacity();
		}
	};

	Int capacity(Int stream) const {
		return Dispatcher::dispatchRtn(stream, &allocator_, CapacityFn());
	}

	static Int capacity(const Int* sizes, Int stream) {
		return 0;
	}


	struct CapacitiesFn {
		template <Int StreamIndex, typename Tree>
		void operator()(const Tree* tree, Position* pos)
		{
			pos->value(StreamIndex) = tree->capacity();
		}
	};

	Position capacities() const
	{
		Position pos;
		Dispatcher::dispatchAll(&allocator_, CapacitiesFn(), &pos);
		return pos;
	}




    struct SizeFn {
    	typedef Int ResultType;

    	template <Int Idx, typename Tree>
    	ResultType operator()(const Tree* tree)
    	{
    		return tree->size();
    	}
    };

    Int size(Int stream) const
    {
    	return Dispatcher::dispatchRtn(stream, &allocator_, SizeFn());
    }

    struct SizesFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, Position* pos)
    	{
    		pos->value(Idx) = tree->size();
    	}
    };

    Position sizes() const
    {
    	Position pos;
    	Dispatcher::dispatchAll(&allocator_, SizesFn(), &pos);
    	return pos;
    }


    struct MaxSizeFn {
    	typedef Int ResultType;

    	template <Int Idx, typename Tree>
    	ResultType operator()(const Tree* tree)
    	{
    		return tree->max_size();
    	}
    };

    Int max_size(Int stream) const
    {
    	return Dispatcher::dispatchRtn(stream, &allocator_, MaxSizeFn());
    }

    struct MaxSizesFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, Position* pos)
    	{
    		pos->value(Idx) = tree->max_size();
    	}
    };

    Position max_sizes() const
    {
    	Position pos;
    	Dispatcher::dispatchAll(&allocator_, MaxSizesFn(), &pos);
    	return pos;
    }


    struct IncSizesFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, const Position* sizes)
    	{
    		tree->size() += sizes->value(Idx);
    	}
    };


    void inc_size(const Position& sizes)
    {
    	Dispatcher::dispatchAll(&allocator_, IncSizesFn(), &sizes);
    }


    bool isEmpty(Int stream) const
    {
    	return size(stream) == 0;
    }

    bool isEmpty() const
    {
    	Position sizes = this->sizes();
    	return sizes.eqAll(0);
    }


    bool isAfterEnd(const Position& idx) const
    {
    	Position sizes = this->sizes();
    	return idx.gteAll(sizes);
    }


    struct InsertSpaceFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, const Position* room_start, const Position* room_length)
    	{
    		tree->insertSpace(room_start->value(Idx), room_length->value(Idx));

    		for (Int c = room_start->value(Idx); c < room_start->value(Idx) + room_length->value(Idx); c++)
    		{
    			tree->clearValues(c);
    		}
    	}
    };

    void insertSpace(const Position& room_start, const Position& room_length)
    {
    	Dispatcher::dispatchAll(&allocator_, InsertSpaceFn(), &room_start, &room_length);
    }

    struct RemoveSpaceFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, const Position* room_start, const Position* room_length)
    	{
    		tree->removeSpace(room_start->value(Idx), room_length->value(Idx));
    	}
    };


    Accumulator removeSpace(const Position& room_start, const Position& room_length, bool reindex = true)
    {
    	Accumulator accum = sum(room_start, room_start + room_length);

    	Dispatcher::dispatchAll(&allocator_, RemoveSpaceFn(), room_start, room_length);

    	if (reindex)
    	{
    		this->reindex();
    	}

    	return accum;
    }

    bool shouldMergeWithSiblings() const
    {
    	return capacities().getAll(sizes());
    }

    bool canMergeWith(const MyType* target) const
    {
    	Position size 		= this->sizes();
    	Position capacity 	= target->capacities();

    	return size.lteAll(capacity);
    }


    struct CopyToFn {
    	template <Int Idx, typename Tree>
    	void operator()(
    			const Tree* tree,
    			MyType* other,
    			const Position& copy_from,
    			const Position& count,
    			const Position& copy_to
    	)
    	{
    		tree->copyTo(other->template get<Tree>(Idx), copy_from.value(Idx), count.value(Idx), copy_to.value(Idx));
    	}
    };


    void copyTo(MyType* other, const Position& copy_from, const Position& count, const Position& copy_to) const
    {
    	MEMORIA_ASSERT_TRUE((copy_from + count).lteAll(sizes()));
    	MEMORIA_ASSERT_TRUE((copy_to + count).lteAll(other->max_sizes()));

    	Dispatcher::dispatchAll(&allocator_, CopyToFn(), other, copy_from, count, copy_to);
    }

    void mergeWith(MyType* target)
    {
    	Position sizes = this->sizes();
    	copyTo(target, Position(0), sizes, target->sizes());
    	target->inc_sizes(sizes);
    	target->reindex();
    }



    struct SumFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, const Position* start, const Position* end, Accumulator* accum)
    	{
    		std::get<Idx>(*accum) += tree->sum(start->value(Idx), end->value(Idx));
    	}
    };

    void sum(const Position* start, const Position* end, Accumulator& accum) const
    {
    	Dispatcher::dispatchAll(&allocator_, SumFn(), start, end, &accum);
    }

    void sum(const Position& start, const Position& end, Accumulator& accum) const
    {
    	Dispatcher::dispatchAll(&allocator_, SumFn(), &start, &end, &accum);
    }

    Accumulator sum(const Position* start, const Position* end) const
    {
    	Accumulator accum;
    	Dispatcher::dispatchAll(&allocator_, SumFn(), start, end, &accum);
    	return accum;
    }

    Accumulator sum(const Position& start, const Position& end) const
    {
    	Accumulator accum;
    	Dispatcher::dispatchAll(&allocator_, SumFn(), &start, &end, &accum);
    	return accum;
    }

//    struct MaxKeysFn {
//    	template <Int Idx, typename Tree>
//    	void operator()(const Tree* tree, Accumulator* acc)
//    	{
//    		//std::get<Idx>(*acc)[0] = tree->sum();
//    	}
//    };

    Accumulator maxKeys() const
    {
    	Accumulator acc;

    	//Dispatcher::dispatchAll(&allocator_, MaxKeysFn(), &acc);

    	return acc;
    }



    Accumulator moveElements(MyType* tgt, const Position& from, const Position& shift)
    {
    	Accumulator result;

    	Position count = this->sizes() - from;

    	sum(from, from + count, result);

    	tgt->insertSpace(Position(0), count + shift);

    	copyTo(tgt, from, count, shift);
    	clear(from, from + count);

    	inc_size(-count);
    	tgt->inc_size(count + shift);

    	tgt->clear(Position(0), shift);

    	reindex();
    	tgt->reindex();

    	return result;
    }

    bool checkCapacities(const Position& sizes) const
    {
    	return capacities().gteAll(sizes);
    }

    struct GenerateDataEventsFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, IPageDataEventHandler* handler)
    	{
    		tree->generateDataEvents(handler);
    	}
    };

    struct InsertSourceFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
    	{
    		tree->insert(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
    	}
    };

    void insert(ISource& src, const Position& pos, const Position& sizes)
    {
    	Dispatcher::dispatchAll(&allocator_, InsertSourceFn(), &src, &pos, &sizes);
    }

    struct UpdateSourceFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
    	{
    		tree->update(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
    	}
    };

    void update(ISource& src, const Position& pos, const Position& sizes)
    {
    	Dispatcher::dispatchAll(&allocator_, UpdateSourceFn(), &src, &pos, &sizes);
    }


    struct ReadToTargetFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
    	{
    		tree->read(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
    	}
    };

    void read(ISource& src, const Position& pos, const Position& sizes)
    {
    	Dispatcher::dispatchAll(&allocator_, ReadToTargetFn(), &src, &pos, &sizes);
    }




    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        Dispatcher::dispatchAll(&allocator_, GenerateDataEventsFn(), handler);
    }

    struct SerializeFn {
    	template <Int Idx, typename Tree>
    	void operator()(const Tree* tree, SerializationData* buf)
    	{
    		tree->serialize(*buf);
    	}
    };

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        allocator_.serialize(buf);

        Dispatcher::dispatchAll(&allocator_, SerializeFn(), &buf);
    }

    struct DeserializeFn {
    	template <Int Idx, typename Tree>
    	void operator()(Tree* tree, DeserializationData* buf)
    	{
    		tree->deserialize(*buf);
    	}
    };

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        allocator_.deserialize(buf);

        Dispatcher::dispatchAll(&allocator_, DeserializeFn(), &buf);
    }
};


}


namespace balanced_tree {

//template <typename Types, bool root1, bool leaf1, bool root2, bool leaf2>
//void ConvertNodeToRoot(
//	const TreeNode<TreeMapNode, Types, root1, leaf1>* src,
//	TreeNode<TreeMapNode, Types, root2, leaf2>* tgt
//)
//{
////	typedef TreeNode<TreeMapNode, Types, root2, leaf2> RootType;
////
////	tgt->init(src->page_size());
////	tgt->copyFrom(src);
////
////	tgt->set_root(true);
////
////	tgt->page_type_hash()   = RootType::hash();
////
////	src->transferDataTo(tgt);
////
////	tgt->set_children_count(src->children_count());
////
////	tgt->clearUnused();
////
////	tgt->reindex();
//}

template <typename Types, bool root1, bool leaf1, bool root2, bool leaf2>
void ConvertRootToNode(
	const TreeNode<mvector2::TreeLeafNode, Types, root1, leaf1>* src,
	TreeNode<mvector2::TreeLeafNode, Types, root2, leaf2>* tgt
)
{
//	typedef TreeNode<TreeMapNode, Types, root2, leaf2> NonRootNode;
//
//	tgt->init(src->page_size());
//	tgt->copyFrom(src);
//	tgt->page_type_hash()   = NonRootNode::hash();
//	tgt->set_root(false);
//
//	src->transferDataTo(tgt);
//
//	tgt->set_children_count(src->children_count());
//
//	tgt->clearUnused();
//
//	tgt->reindex();
}



}



template <typename Types, bool root, bool leaf>
struct TypeHash<mvector2::TreeLeafNode<Types, root, leaf> > {

	typedef mvector2::TreeLeafNode<Types, root, leaf> Node;

    static const UInt Value = HashHelper<
    		TypeHash<typename Node::Base>::Value,
    		Node::VERSION,
    		root,
    		leaf,
    		Types::Indexes,
    		TypeHash<typename Types::Name>::Value
    >::Value;
};


}

#endif
