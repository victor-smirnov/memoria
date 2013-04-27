
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_BALANCEDTREE_NODES_LEAFNODE_HPP
#define _MEMORIA_PROTOTYPES_BALANCEDTREE_NODES_LEAFNODE_HPP

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
namespace balanced_tree	{




template <
	typename K,
	typename V
>
struct TreeLeafNodeTypes: PackedFSETreeTypes<K, K, V> {

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


	typedef TreeLeafNodeTypes<
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

    static const Int Streams 													= ListSize<StreamsStructList>::Value;

    TreeLeafNode(): Base() {}

private:
	struct InitFn {
		Int block_size(Int items_number) const
		{
			Position sizes;
			sizes[0] = items_number;
			return MyType::block_size(sizes);
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

	PackedAllocator* allocator() {
		return &allocator_;
	}

	const PackedAllocator* allocator() const {
		return &allocator_;
	}

	bool is_empty(Int idx) const
	{
		return allocator_.is_empty(idx);
	}



private:
	struct BlockSizeFn {
		Int size_ = 0;

		template <Int StreamIndex, typename Node>
		void stream(Node*, const Position& sizes)
		{
			if (sizes[StreamIndex] > 0)
			{
				size_ += Node::block_size(sizes[StreamIndex]);
			}
		}
	};

public:
	static Int block_size(const Position& sizes)
	{
		BlockSizeFn fn;

		Dispatcher::dispatchAllStatic(fn, sizes);

		Int client_area = fn.size_;

		return PackedAllocator::block_size(client_area, Streams);
	}


	void init(Int block_size, const Position& sizes)
	{
		init0(block_size - sizeof(Me) + sizeof(allocator_), sizes);
	}

	void init(Int block_size)
	{
		init0(block_size - sizeof(Me) + sizeof(allocator_));
	}

	void prepare(Int block_size)
	{
		allocator_.init(block_size - sizeof(Me) + sizeof(allocator_), Streams);
	}

	void layout(const Position& sizes)
	{
		Int block_size = this->page_size();
		init(block_size, sizes);
	}

	UBigInt active_streams() const
	{
		UBigInt streams = -1ull;
		for (Int c = 0; c < Streams; c++)
		{
			UBigInt bit = !allocator_.is_empty(c);
			streams |= bit << c;
		}

		return streams;
	}

private:

	struct InitStructFn {
		template <Int StreamIndex, typename Tree>
		void stream(Tree*, PackedAllocator* allocator, const Position& sizes)
		{
			if (sizes[StreamIndex] > 0)
			{
				Int block_size = Tree::block_size(sizes[StreamIndex]);
				allocator->template allocate<Tree>(StreamIndex, block_size);
			}
		}

		template <Int StreamIndex, typename Tree>
		void stream(Tree*, PackedAllocator* allocator, Int client_area)
		{
			if (StreamIndex == 0)
			{
				allocator->template allocate<Tree>(StreamIndex, client_area);
			}
		}
	};

public:

	void init0(Int block_size, const Position& sizes)
	{
		allocator_.init(block_size, Streams);

		Dispatcher::dispatchAllStatic(InitStructFn(), &allocator_, sizes);
	}

	void init0(Int block_size)
	{
		allocator_.init(block_size, Streams);

		Dispatcher::dispatchAllStatic(InitStructFn(), &allocator_, allocator_.client_area());
	}


	struct ObjectSizeFn {
		template <Int StreamIndex, typename Tree>
		void stream(const Tree* tree, PackedAllocator* allocator, const Int* size)
		{
			*size += tree->object_size();
		}
	};


	Int object_size() const
	{
		Int size = 0;
		Dispatcher::dispatchNotEmpty(ObjectSizeFn(), &allocator_, &size);
		return size;
	}



	void clear(const Position&, const Position&)
	{

	}


    void clearUnused() {}

    struct ReindexFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree)
    	{
    		tree->reindex();
    	}
    };

    void reindex()
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, ReindexFn());
    }


    template <typename TreeType>
    struct TransferToFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, TreeType* other)
    	{
    		tree->transferDataTo(other->template get<Tree>(Idx));
    	}
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, TransferToFn<TreeType>(), other);
    }

    Int data_size() const
    {
        return sizeof(Me) + this->getDataSize();
    }



	struct CapacityFn {
		typedef Int ResultType;

		template <Int StreamIndex, typename Tree>
		ResultType stream(const Tree* tree)
		{
			return tree->capacity();
		}
	};

	Int capacity(Int stream) const {
		return Dispatcher::dispatchRtn(stream, &allocator_, CapacityFn());
	}

	static Int capacity(Int block_size, const Int* sizes, Int stream)
	{
		return FindTotalElementsNumber2(block_size - sizeof(Me) + sizeof(allocator_), InitFn());
	}

	struct Capacity2Fn {
		template <Int StreamIndex, typename Tree>
		void stream(const Tree*, const Int* sizes, Int* mem_used, Int except)
		{
			Int size = sizes[StreamIndex];

			if (size > 0 && StreamIndex != except)
			{
				mem_used += Tree::block_size(sizes[StreamIndex]);
			}
		}
	};

	struct Capacity3Fn {
		typedef Int ResultType;

		template <Int StreamIndex, typename Tree>
		ResultType stream(const Tree* tree, Int free_mem)
		{
			Int size = tree != nullptr ? tree->size() : 0;

			Int capacity = Tree::elements_for(free_mem) - size;

			return capacity >= 0 ? capacity : 0;
		}
	};


	Int capacity(const Int* sizes, Int stream) const
	{
		Position fillment = this->sizes();

		for (Int c = 0; c < Streams; c++)
		{
			fillment[c] += sizes[c];
		}

		Int mem_used = 0;

		Dispatcher::dispatchAllStatic(Capacity2Fn(), sizes, &mem_used, stream);

		Int client_area	= allocator_.client_area();

		return Dispatcher::dispatchRtn(stream, &allocator_, Capacity3Fn(), client_area - mem_used);
	}


	struct CapacitiesFn {
		template <Int StreamIndex, typename Tree>
		void stream(const Tree* tree, Position* pos)
		{
			pos->value(StreamIndex) = tree->capacity();
		}
	};

	Position capacities() const
	{
		Position pos;
		Dispatcher::dispatchNotEmpty(&allocator_, CapacitiesFn(), &pos);
		return pos;
	}




    struct SizeFn {
    	typedef Int ResultType;

    	template <Int Idx, typename Tree>
    	ResultType stream(const Tree* tree)
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
    	void stream(const Tree* tree, Position* pos)
    	{
    		pos->value(Idx) = tree->size();
    	}
    };

    Position sizes() const
    {
    	Position pos;
    	Dispatcher::dispatchNotEmpty(&allocator_, SizesFn(), &pos);
    	return pos;
    }


    struct MaxSizeFn {
    	typedef Int ResultType;

    	template <Int Idx, typename Tree>
    	ResultType stream(const Tree* tree)
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
    	void stream(const Tree* tree, Position* pos)
    	{
    		pos->value(Idx) = tree->max_size();
    	}
    };

    Position max_sizes() const
    {
    	Position pos;
    	Dispatcher::dispatchNotEmpty(&allocator_, MaxSizesFn(), &pos);
    	return pos;
    }


    struct IncSizesFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, const Position* sizes)
    	{
    		tree->size() += sizes->value(Idx);
    	}
    };


    void inc_size(const Position& sizes)
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, IncSizesFn(), &sizes);
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


    struct InitStreamIfEmpty {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, PackedAllocator* allocator, const Position* sizes)
    	{
    		if (tree == nullptr && sizes->value(Idx) > 0)
    		{
    			allocator->template allocate<Tree>(Idx, Tree::empty_size());
    		}
    	}
    };


    void initStreamsIfEmpty(const Position& sizes)
    {
    	Dispatcher::dispatchAll(&allocator_, InitStreamIfEmpty(), &allocator_, &sizes);
    }


    struct InsertSpaceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, PackedAllocator* allocator, const Position* room_start, const Position* room_length)
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
    	initStreamsIfEmpty(room_length);

    	Dispatcher::dispatchAll(&allocator_, InsertSpaceFn(), &allocator_, &room_start, &room_length);
    }

    struct RemoveSpaceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, const Position* room_start, const Position* room_length)
    	{
    		tree->removeSpace(room_start->value(Idx), room_length->value(Idx));
    	}
    };


    Accumulator removeSpace(const Position& room_start, const Position& room_length, bool reindex = true)
    {
    	Accumulator accum = sum(room_start, room_start + room_length);

    	Dispatcher::dispatchNotEmpty(&allocator_, RemoveSpaceFn(), &room_start, &room_length);

    	if (reindex)
    	{
    		this->reindex();
    	}

    	return accum;
    }

    bool shouldBeMergedWithSiblings() const
    {
    	Position sizes = this->sizes();
    	Int block_size = MyType::block_size(sizes);

    	return block_size <= allocator_.block_size() / 2;
    }

    bool canBeMergedWith(const MyType* other) const
    {
    	Position my_sizes 		= this->sizes();
    	Position other_sizes 	= other->sizes();
    	Position required_sizes	= my_sizes + other_sizes;

    	Int required_block_size = MyType::block_size(required_sizes);

    	return required_block_size <= other->allocator_.block_size();
    }

    struct MergeWithFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, MyType* other)
    	{
    		Int size = tree->size();

    		if (size > 0)
    		{
    			if (other->is_empty(Idx))
    			{
    				Int block_size = Tree::block_size(size);
    				other->allocator()->template allocate<Tree>(Idx, block_size);
    			}

    			Tree* other_tree = other->template get<Tree>(Idx);

    			other_tree->ensureCapacity(size);
    			tree->copyTo(other_tree, 0, size, other_tree->size());

    			other_tree->size() += size;
    			other_tree->reindex();
    		}
    	}
    };

    void mergeWith(MyType* other)
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, MergeWithFn(), other);
    }

    struct SplitToFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, MyType* other, const Position* indexes, const Position* shifts)
    	{
    		Int idx   = indexes->value(Idx);
    		Int shift = shifts->value(Idx);
    		Int size  = tree->size();

    		if (size > 0)
    		{
    			Int remainder 		= size - idx;
    			Int block_size 		= Tree::block_size(remainder);
    			Tree* other_tree 	= other->allocator()->template allocate<Tree>(Idx, block_size);

    			tree->clear(0, shift);

    			tree->copyTo(other_tree, idx, remainder, shift);

    			other_tree->size() += remainder;
    			other_tree->reindex();

    			tree->removeSpace(idx, remainder);
    		}
    	}
    };


    Accumulator splitTo(MyType* other, const Position& from, const Position& shift)
    {
    	Accumulator result;

    	Position sizes = this->sizes();

    	sum(from, sizes, result);

    	Dispatcher::dispatchNotEmpty(&allocator_, SplitToFn(), other, &from, &shift);

    	return result;
    }


    struct CopyToFn {
    	template <Int Idx, typename Tree>
    	void stream(
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

    	Dispatcher::dispatchNotEmpty(&allocator_, CopyToFn(), other, copy_from, count, copy_to);
    }


    struct SumFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, const Position* start, const Position* end, Accumulator* accum)
    	{
    		std::get<Idx>(*accum) += tree->sum(start->value(Idx), end->value(Idx));
    	}
    };

    void sum(const Position* start, const Position* end, Accumulator& accum) const
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, SumFn(), start, end, &accum);
    }

    void sum(const Position& start, const Position& end, Accumulator& accum) const
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, SumFn(), &start, &end, &accum);
    }

    Accumulator sum(const Position* start, const Position* end) const
    {
    	Accumulator accum;
    	Dispatcher::dispatchNotEmpty(&allocator_, SumFn(), start, end, &accum);
    	return accum;
    }

    Accumulator sum(const Position& start, const Position& end) const
    {
    	Accumulator accum;
    	Dispatcher::dispatchNotEmpty(&allocator_, SumFn(), &start, &end, &accum);
    	return accum;
    }

    struct MaxKeysFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, Accumulator* acc)
    	{
    		std::get<Idx>(*acc)[0] = tree->size();
    	}
    };

    Accumulator maxKeys() const
    {
    	Accumulator acc;

    	Dispatcher::dispatchNotEmpty(&allocator_, MaxKeysFn(), &acc);

    	return acc;
    }

    bool checkCapacities(const Position& sizes) const
    {
    	return capacities().gteAll(sizes);
    }

    struct GenerateDataEventsFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, IPageDataEventHandler* handler)
    	{
    		tree->generateDataEvents(handler);
    	}
    };

    struct InsertSourceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
    	{
    		tree->insert(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
    	}
    };

    void insert(ISource& src, const Position& pos, const Position& sizes)
    {
    	initStreamsIfEmpty(sizes);

    	Dispatcher::dispatchNotEmpty(&allocator_, InsertSourceFn(), &src, &pos, &sizes);
    }

    struct UpdateSourceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
    	{
    		tree->update(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
    	}
    };

    void update(ISource* src, const Position& pos, const Position& sizes)
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, UpdateSourceFn(), src, &pos, &sizes);
    }


    struct ReadToTargetFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, ITarget* tgt, const Position* pos, const Position* sizes)
    	{
    		tree->read(tgt->stream(Idx), pos->value(Idx), sizes->value(Idx));
    	}
    };

    void read(ITarget* tgt, const Position& pos, const Position& sizes) const
    {
    	Dispatcher::dispatchNotEmpty(&allocator_, ReadToTargetFn(), tgt, &pos, &sizes);
    }


    template <typename Fn, typename... Args>
    Int find(Int stream, Fn&& fn, Args... args) const
    {
    	return Dispatcher::dispatchRtn(stream, &allocator_, std::move(fn), args...);
    }

    template <typename Fn, typename... Args>
    void process(Int stream, Fn&& fn, Args... args) const
    {
    	Dispatcher::dispatch(stream, &allocator_, std::move(fn), args...);
    }





    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        Dispatcher::dispatchNotEmpty(&allocator_, GenerateDataEventsFn(), handler);
    }

    struct SerializeFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, SerializationData* buf)
    	{
    		tree->serialize(*buf);
    	}
    };

    template <template <typename> class FieldFactory>
    void serialize(SerializationData& buf) const
    {
        Base::template serialize<FieldFactory>(buf);

        allocator_.serialize(buf);

        Dispatcher::dispatchNotEmpty(&allocator_, SerializeFn(), &buf);
    }

    struct DeserializeFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, DeserializationData* buf)
    	{
    		tree->deserialize(*buf);
    	}
    };

    template <template <typename> class FieldFactory>
    void deserialize(DeserializationData& buf)
    {
        Base::template deserialize<FieldFactory>(buf);

        allocator_.deserialize(buf);

        Dispatcher::dispatchNotEmpty(&allocator_, DeserializeFn(), &buf);
    }

    static void InitType() {
    	int a = 0; a++;
    }

    void set_children_count(Int) {
    	throw Exception(MA_SRC, "Deprecated method set_children_count()");
    }


    struct DumpFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree)
    	{
    		tree->dump();
    	}
    };


    void dump() const {
    	Dispatcher::dispatchNotEmpty(&allocator_, DumpFn());
    }
};




template <typename Types, bool root1, bool leaf1, bool root2, bool leaf2>
void ConvertNodeToRoot(
	const TreeNode<TreeLeafNode, Types, root1, leaf1>* src,
	TreeNode<TreeLeafNode, Types, root2, leaf2>* tgt
)
{
	typedef TreeNode<TreeLeafNode, Types, root2, leaf2> RootType;

	tgt->copyFrom(src);

	tgt->set_root(true);

	tgt->page_type_hash()   = RootType::hash();

	tgt->init(src->page_size());

	src->transferDataTo(tgt);

	tgt->clearUnused();

	tgt->reindex();
}

template <typename Types, bool root1, bool leaf1, bool root2, bool leaf2>
void ConvertRootToNode(
	const TreeNode<TreeLeafNode, Types, root1, leaf1>* src,
	TreeNode<TreeLeafNode, Types, root2, leaf2>* tgt
)
{
	typedef TreeNode<TreeLeafNode, Types, root2, leaf2> NonRootNode;

	tgt->copyFrom(src);

	tgt->page_type_hash() = NonRootNode::hash();

	tgt->set_root(false);

	tgt->init(src->page_size());

	src->transferDataTo(tgt);

	tgt->clearUnused();

	tgt->reindex();
}



}



template <typename Types, bool root, bool leaf>
struct TypeHash<balanced_tree::TreeLeafNode<Types, root, leaf> > {

	typedef balanced_tree::TreeLeafNode<Types, root, leaf> Node;

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
