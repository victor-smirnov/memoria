
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

    static const Int Streams 													= ListSize<StreamsStructList>::Value;

    TreeLeafNode(): Base() {}

private:


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

private:

	struct InitStructFn {
		template <Int StreamIndex, typename Tree>
		void stream(Tree*, PackedAllocator* allocator, const Position& sizes)
		{
			if (sizes[StreamIndex] > 0)
			{
				allocator->template allocate<Tree>(StreamIndex, sizes[StreamIndex]);
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


	void clear(const Position&, const Position&)
	{

	}


    void clearUnused() {}



    void reindex()
    {
        struct ReindexFn {
        	template <Int Idx, typename Tree>
        	void stream(Tree* tree)
        	{
        		tree->reindex();
        	}
        };

    	Dispatcher::dispatchAll(&allocator_, ReindexFn());
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
    	Dispatcher::dispatchAll(&allocator_, TransferToFn<TreeType>(), other);
    }

    Int data_size() const
    {
        return sizeof(Me) + this->getDataSize();
    }





	Int capacity(Int stream) const
	{
		struct Fn {
			typedef Int ResultType;

			template <Int StreamIndex, typename Tree>
			ResultType stream(const Tree* tree)
			{
				return tree->capacity();
			}
		};

		return Dispatcher::dispatchRtn(stream, &allocator_, Fn());
	}


	static Int capacity(Int block_size, const Int* sizes, Int stream)
	{
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

		return FindTotalElementsNumber2(block_size - sizeof(Me) + sizeof(allocator_), InitFn());
	}


	Position capacities() const
	{
		struct CapacitiesFn {
			template <Int StreamIndex, typename Tree>
			void stream(const Tree* tree, Position* pos)
			{
				pos->value(StreamIndex) = tree->capacity();
			}
		};

		Position pos;
		Dispatcher::dispatchAll(&allocator_, CapacitiesFn(), &pos);
		return pos;
	}






    Int size(Int stream) const
    {
        struct {
        	typedef Int ResultType;

        	template <Int Idx, typename Tree>
        	Int stream(const Tree* tree)
        	{
        		return tree->size();
        	}
        } fn;

    	return Dispatcher::dispatchRtn(stream, &allocator_, fn);
    }



    Position sizes() const
    {
    	struct {
    		template <Int Idx, typename Tree>
    		void stream(const Tree* tree, Position* pos)
    		{
    			pos->value(Idx) = tree->size();
    		}
    	} fn;


    	Position pos;
    	Dispatcher::dispatchAll(&allocator_, fn, &pos);
    	return pos;
    }


    Int max_size(Int stream) const
    {
        struct {
        	typedef Int ResultType;

        	template <Int Idx, typename Tree>
        	ResultType stream(const Tree* tree)
        	{
        		return tree->max_size();
        	}
        } fn;

    	return Dispatcher::dispatchRtn(stream, &allocator_, fn);
    }



    Position max_sizes() const
    {
    	struct {
    		template <Int Idx, typename Tree>
    		void stream(const Tree* tree, Position* pos)
    		{
    			pos->value(Idx) = tree->max_size();
    		}
    	} fn;


    	Position pos;
    	Dispatcher::dispatchAll(&allocator_, fn, &pos);
    	return pos;
    }


    void inc_size(const Position& sizes)
    {
    	struct {
    		template <Int Idx, typename Tree>
    		void stream(Tree* tree, const Position* sizes)
    		{
    			tree->size() += sizes->value(Idx);
    		}
    	} fn;

    	Dispatcher::dispatchAll(&allocator_, fn, &sizes);
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


    void insertSpace(const Position& room_start, const Position& room_length)
    {
    	struct InsertSpaceFn {
    		template <Int Idx, typename Tree>
    		void stream(Tree* tree, const Position* room_start, const Position* room_length)
    		{
    			tree->insertSpace(room_start->value(Idx), room_length->value(Idx));

    			for (Int c = room_start->value(Idx); c < room_start->value(Idx) + room_length->value(Idx); c++)
    			{
    				tree->clearValues(c);
    			}
    		}
    	};


    	Dispatcher::dispatchAll(&allocator_, InsertSpaceFn(), &room_start, &room_length);
    }




    Accumulator removeSpace(const Position& room_start, const Position& room_length, bool reindex = true)
    {
    	struct RemoveSpaceFn {
    		template <Int Idx, typename Tree>
    		void stream(Tree* tree, const Position* room_start, const Position* room_length)
    		{
    			tree->removeSpace(room_start->value(Idx), room_length->value(Idx));
    		}
    	};

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





    void copyTo(MyType* other, const Position& copy_from, const Position& count, const Position& copy_to) const
    {
    	MEMORIA_ASSERT_TRUE((copy_from + count).lteAll(sizes()));
    	MEMORIA_ASSERT_TRUE((copy_to + count).lteAll(other->max_sizes()));

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
    	void stream(const Tree* tree, const Position* start, const Position* end, Accumulator* accum)
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



    Accumulator maxKeys() const
    {
    	struct MaxKeysFn {
    		template <Int Idx, typename Tree>
    		void stream(const Tree* tree, Accumulator* acc)
    		{
    			std::get<Idx>(*acc)[0] = tree->size();
    		}
    	};

    	Accumulator acc;

    	Dispatcher::dispatchAll(&allocator_, MaxKeysFn(), &acc);

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

    	tgt->clear(Position(0), shift);

    	reindex();
    	tgt->reindex();

    	return result;
    }

    bool checkCapacities(const Position& sizes) const
    {
    	return capacities().gteAll(sizes);
    }



    void insert(ISource& src, const Position& pos, const Position& sizes)
    {
    	struct InsertSourceFn {
    		template <Int Idx, typename Tree>
    		void stream(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
    		{
    			tree->insert(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
    		}
    	};

    	Dispatcher::dispatchAll(&allocator_, InsertSourceFn(), &src, &pos, &sizes);
    }


    void update(ISource* src, const Position& pos, const Position& sizes)
    {
        struct UpdateSourceFn {
        	template <Int Idx, typename Tree>
        	void stream(Tree* tree, ISource* src, const Position* pos, const Position* sizes)
        	{
        		tree->update(src->stream(Idx), pos->value(Idx), sizes->value(Idx));
        	}
        };

    	Dispatcher::dispatchAll(&allocator_, UpdateSourceFn(), src, &pos, &sizes);
    }


    void read(ITarget* tgt, const Position& pos, const Position& sizes) const
    {
    	struct ReadToTargetFn {
    		template <Int Idx, typename Tree>
    		void stream(Tree* tree, ITarget* tgt, const Position* pos, const Position* sizes)
    		{
    			tree->read(tgt->stream(Idx), pos->value(Idx), sizes->value(Idx));
    		}
    	};

    	Dispatcher::dispatchAll(&allocator_, ReadToTargetFn(), tgt, &pos, &sizes);
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

        struct GenerateDataEventsFn {
        	template <Int Idx, typename Tree>
        	void stream(const Tree* tree, IPageDataEventHandler* handler)
        	{
        		tree->generateDataEvents(handler);
        	}
        };

        Dispatcher::dispatchAll(&allocator_, GenerateDataEventsFn(), handler);
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

        Dispatcher::dispatchAll(&allocator_, SerializeFn(), &buf);
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

        Dispatcher::dispatchAll(&allocator_, DeserializeFn(), &buf);
    }

    static void InitType() {
    	int a = 0; a++;
    }

    void set_children_count(Int) {
    	throw Exception(MA_SRC, "Deprecated method set_children_count()");
    }
};


}


namespace balanced_tree {

template <typename Types, bool root1, bool leaf1, bool root2, bool leaf2>
void ConvertNodeToRoot(
	const TreeNode<mvector2::TreeLeafNode, Types, root1, leaf1>* src,
	TreeNode<mvector2::TreeLeafNode, Types, root2, leaf2>* tgt
)
{
	typedef TreeNode<mvector2::TreeLeafNode, Types, root2, leaf2> RootType;

	tgt->copyFrom(src);

	tgt->set_root(true);

	tgt->page_type_hash() = RootType::hash();

	tgt->init(src->page_size());

	src->transferDataTo(tgt);

	tgt->clearUnused();

	tgt->reindex();
}

template <typename Types, bool root1, bool leaf1, bool root2, bool leaf2>
void ConvertRootToNode(
	const TreeNode<mvector2::TreeLeafNode, Types, root1, leaf1>* src,
	TreeNode<mvector2::TreeLeafNode, Types, root2, leaf2>* tgt
)
{
	typedef TreeNode<mvector2::TreeLeafNode, Types, root2, leaf2> NonRootNode;

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
