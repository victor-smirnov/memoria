
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

#include <memoria/core/packed/tree/packed_fse_tree.hpp>
#include <memoria/core/packed/tree/packed_vle_tree.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_vle_map.hpp>

#include <memoria/core/packed/array/packed_fse_array.hpp>
#include <memoria/core/packed/tools/packed_allocator.hpp>
#include <memoria/core/packed/tools/packed_dispatcher.hpp>

#include <memoria/prototypes/bt/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_names.hpp>
#include <memoria/prototypes/bt/nodes/branch_node.hpp>


namespace memoria	{
namespace bt		{




template <
	typename K,
	typename V
>
struct TreeLeafNodeTypes: Packed2TreeTypes<V, K> {

};

template <typename Types>
struct LeafNodeStreamTypes: Types {
	static const bool Leaf = true;
};


template <
	typename Types
>
class LeafNode: public TreeNodeBase<typename Types::Metadata, typename Types::NodeBase>
{

    static const Int  BranchingFactor                                           = PackedTreeBranchingFactor;

    typedef LeafNode<Types>                                      				Me;
    typedef LeafNode<Types>                                      				MyType;

public:
    static const UInt VERSION                                                   = 2;

    static const bool Leaf 														= true;

    typedef bt::TreeNodeBase<
    			typename Types::Metadata,
    			typename Types::NodeBase
    >  																			Base;

private:



public:

    typedef typename Types::Accumulator											Accumulator;
    typedef typename Types::Position											Position;

    template <
        	template <typename> class,
        	typename
    >
    friend class NodePageAdaptor;

    typedef LeafNodeStreamTypes<Types> 											StreamTypes;

	typedef typename PackedStructListBuilder<
				StreamTypes,
	    		typename Types::StreamDescriptors,
	    		0
	>::LeafStructList															StreamsStructList;

	typedef typename PackedDispatcherTool<StreamsStructList>::Type				Dispatcher;

    static const Int Streams 													= ListSize<StreamsStructList>::Value;

    LeafNode(): Base() {}

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

	PackedAllocator* allocator()
	{
		return Base::allocator()->template get<PackedAllocator>(Base::STREAMS);
	}

	const PackedAllocator* allocator() const
	{
		return Base::allocator()->template get<PackedAllocator>(Base::STREAMS);
	}

	template <typename T>
	T* get(Int idx)
	{
		return allocator()->template get<T>(idx);
	}

	template <typename T>
	const T* get(Int idx) const
	{
		return allocator()->template get<T>(idx);
	}

	bool is_empty(Int idx) const
	{
		return allocator()->is_empty(idx);
	}

	bool is_empty() const
	{
		for (Int c = 0; c < Streams; c++)
		{
			if (!allocator()->is_empty(c)) {
				return false;
			}
		}

		return true;
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


	static Int client_area(Int block_size, bool root)
	{
		Int free_space = Base::free_space(block_size, root);
		return PackedAllocator::client_area(free_space, Streams);
	}

	Int total_size() const
	{
		return allocator()->allocated();
	}

	void prepare()
	{
		Base::init();
		Base::allocator()->allocateAllocator(Base::STREAMS, Streams);
	}

	void layout(const Position& sizes)
	{
		UBigInt streams = 0;

		for (Int c = 0; c < Streams; c++)
		{
			streams |= 1<<c;
		}

		layout(streams);
	}


	struct LayoutFn {
		template <Int StreamIndex, typename Stream>
		void stream(Stream*, PackedAllocator* alloc, UBigInt streams)
		{
			if (streams & (1<<StreamIndex))
			{
				if (alloc->is_empty(StreamIndex))
				{
					alloc->template allocateEmpty<Stream>(StreamIndex);
				}
			}
		}
	};


	void layout(UBigInt streams)
	{
		Dispatcher::dispatchAllStatic(LayoutFn(), this->allocator(), streams);
	}


	UBigInt active_streams() const
	{
		UBigInt streams = 0;
		for (Int c = 0; c < Streams; c++)
		{
			UBigInt bit = !allocator()->is_empty(c);
			streams += (bit << c);
		}

		return streams;
	}

private:

	struct InitStructFn {
		template <Int StreamIndex, typename Tree>
		void stream(Tree*, PackedAllocator* allocator, const Position& sizes)
		{
			if (sizes[StreamIndex] > -1)
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
		allocator()->init(block_size, Streams);

		Dispatcher::dispatchAllStatic(InitStructFn(), allocator(), sizes);
	}

	void init0(Int block_size)
	{
		allocator()->init(block_size, Streams);

		Dispatcher::dispatchAllStatic(InitStructFn(), allocator(), allocator()->client_area());
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
		Dispatcher::dispatchNotEmpty(ObjectSizeFn(), allocator(), &size);
		return size;
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
    	Dispatcher::dispatchNotEmpty(allocator(), ReindexFn());
    }

    struct CheckFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree)
    	{
    		tree->check();
    	}
    };

    void check() const
    {
    	Dispatcher::dispatchNotEmpty(allocator(), CheckFn());
    }


    template <typename TreeType>
    struct TransferToFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, TreeType* other)
    	{
    		auto allocator = tree->allocator();
    		auto other_allocator = other->allocator();

    		other_allocator->importBlock(Idx, allocator, Idx);
    	}
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
    	Base::transferDataTo(other);

    	Dispatcher::dispatchNotEmpty(allocator(), TransferToFn<TreeType>(), other);
    }



    Int data_size() const
    {
        return sizeof(Me) + this->getDataSize();
    }



	struct MemUsedFn {
		template <Int StreamIndex, typename Tree>
		void stream(const Tree* tree, const Position* sizes, Int* mem_used, Int except)
		{
			if (StreamIndex != except)
			{
				Int size = sizes->value(StreamIndex);

				if (tree != nullptr || size > 0)
				{
					*mem_used += Tree::block_size(size);
				}
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


	Int capacity(const Position& sizes, Int stream) const
	{
		Position fillment = this->sizes();

		for (Int c = 0; c < Streams; c++)
		{
			fillment[c] += sizes[c];
		}

		Int mem_used = 0;
		Dispatcher::dispatchAll(allocator(), MemUsedFn(), &fillment, &mem_used, stream);

		Int client_area	= MyType::client_area(this->page_size(), this->is_root());

		return Dispatcher::dispatchRtn(stream, allocator(), Capacity3Fn(), client_area - mem_used);
	}

	Int capacity(const Int* sizes, Int stream) const
	{
		Position psizes;
		for (Int c = 0; c < Streams; c++) psizes[c] = sizes[c];

		return capacity(psizes, stream);
	}

	struct StaticCapacity3Fn {
		typedef Int ResultType;

		template <Int StreamIndex, typename Tree>
		ResultType stream(const Tree* tree, Int free_mem)
		{
			Int size = tree != nullptr ? tree->size() : 0;

			Int capacity = Tree::elements_for(free_mem) - size;

			return capacity >= 0 ? capacity : 0;
		}
	};

	static Int capacity(Int block_size, const Int* sizes, Int stream, bool root)
	{
		Position fillment;

		for (Int c = 0; c < Streams; c++)
		{
			fillment[c] = sizes[c];
		}

		Int mem_used = 0;
		Dispatcher::dispatchAllStatic(MemUsedFn(), &fillment, &mem_used, stream);

		Int client_area	= MyType::client_area(block_size, root);

		return Dispatcher::dispatchStaticRtn(stream, Capacity3Fn(), client_area - mem_used);
	}



	Int capacity(Int stream) const
	{
		Position sizes = this->sizes();
		return capacity(sizes, stream);
	}



	struct CheckCapacitiesFn {

		template <Int StreamIdx, typename Tree>
		void stream(const Tree* tree, const Position* sizes, Int* mem_size)
		{
			Int size = sizes->value(StreamIdx);

			if (tree != nullptr || size > 0)
			{
				*mem_size += Tree::block_size(size);
			}
		}
	};

	bool checkCapacities(const Position& sizes) const
	{
		if (DebugCounter) {
			int a = 0; a++;
		}

		Position fillment = this->sizes();

		for (Int c = 0; c < Streams; c++)
		{
			fillment[c] += sizes[c];
		}

		Int mem_size = 0;

		Dispatcher::dispatchAll(allocator(), CheckCapacitiesFn(), &fillment, &mem_size);

		Int free_space 		= Base::free_space(this->page_size(), this->is_root());
		Int client_area		= PackedAllocator::client_area(free_space, Streams);

		return client_area >= mem_size;
	}




    struct SizeFn {
    	typedef Int ResultType;

    	template <Int Idx, typename Tree>
    	ResultType stream(const Tree* tree)
    	{
    		return tree != nullptr ? tree->size() : 0;
    	}
    };

    Int size(Int stream) const
    {
    	return Dispatcher::dispatchRtn(stream, allocator(), SizeFn());
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
    	Dispatcher::dispatchNotEmpty(allocator(), SizesFn(), &pos);
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
    	return Dispatcher::dispatchRtn(stream, allocator(), MaxSizeFn());
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
    	Dispatcher::dispatchNotEmpty(allocator(), MaxSizesFn(), &pos);
    	return pos;
    }


    struct MaxOfSizesFn {
    	Int max_size_ = 0;

    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree)
    	{
    		if (tree->size() > max_size_)
    		{
    			max_size_ = tree->size();
    		}
    	}
    };

    Int maxOfSizes() const
    {
    	MaxOfSizesFn fn;
    	Dispatcher::dispatchNotEmpty(allocator(), fn);
    	return fn.max_size_;
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


    bool isAfterEnd(const Position& idx, UBigInt active_streams) const
    {
    	Position sizes = this->sizes();

    	for (Int c = 0; c < Streams; c++)
    	{
    		if (active_streams & (1<<c) && idx[c] < sizes[c])
    		{
    			return false;
    		}
    	}

    	return true;

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
    	Dispatcher::dispatchAll(allocator(), InitStreamIfEmpty(), allocator(), &sizes);
    }


    struct InsertSpaceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, PackedAllocator* allocator, const Position* room_start, const Position* room_length)
    	{
    		if (tree != nullptr)
    		{
    			tree->insertSpace(room_start->value(Idx), room_length->value(Idx));
    		}
    		else {
    			MEMORIA_ASSERT_TRUE(room_length->value(Idx) == 0);
    		}
    	}
    };

    void insertSpace(const Position& room_start, const Position& room_length)
    {
    	initStreamsIfEmpty(room_length);

    	Dispatcher::dispatchAll(allocator(), InsertSpaceFn(), allocator(), &room_start, &room_length);
    }

    void insertSpace(Int stream, Int room_start, Int room_length)
    {
       	insertSpace(Position::create(stream, room_start), Position::create(stream, room_length));
    }

    struct RemoveSpaceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, const Position* room_start, const Position* room_end)
    	{
    		tree->removeSpace(room_start->value(Idx), room_end->value(Idx));
    	}

    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, Int room_start, Int room_end)
    	{
    		if (tree != nullptr) {
    			tree->removeSpace(room_start, room_end);
    		}
    	}
    };

    Accumulator removeSpace(Int stream, Int room_start, Int room_end)
    {
    	Accumulator accum;
    	this->sums(stream, room_start, room_end, accum);

    	Dispatcher::dispatch(stream, allocator(), RemoveSpaceFn(), room_start, room_end);

    	removeEmptyStreams();

    	return accum;
    }

    Accumulator removeSpace(const Position& room_start, const Position& room_end)
    {
    	Accumulator accum;
    	this->sums(room_start, room_end, accum);

    	Dispatcher::dispatchNotEmpty(allocator(), RemoveSpaceFn(), &room_start, &room_end);

    	removeEmptyStreams();

    	return accum;
    }

    void removeEmptyStreams()
    {
    	Position sizes = this->sizes();

    	for (Int c = Position::Indexes - 1; c >= 0; c--)
    	{
    		if (sizes[c] == 0)
    		{
    			allocator()->free(c);
    		}
    	}
    }

    struct CanMergeWithFn {
    	Int mem_used_ = 0;

    	template <Int StreamIdx, typename Tree>
    	void stream(const Tree* tree, const MyType* other)
    	{
    		if (tree != nullptr)
    		{
    			if (other->allocator()->is_empty(StreamIdx))
    			{
    				mem_used_ += tree->block_size();
    			}
    			else {
    				const Tree* other_tree = other->allocator()->template get<Tree>(StreamIdx);
    				mem_used_ += tree->block_size(other_tree);
    			}
    		}
    		else {
    			if (!other->allocator()->is_empty(StreamIdx))
    			{
    				Int element_size = other->allocator()->element_size(StreamIdx);
    				mem_used_ += element_size;
    			}
    		}
    	}
    };

    bool canBeMergedWith(const MyType* other) const
    {
    	CanMergeWithFn fn;
    	Dispatcher::dispatchAll(allocator(), fn, other);

    	Int free_space = this->allocator()->free_space();

    	return free_space >= fn.mem_used_;
    }



    struct MergeWithFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, MyType* other)
    	{
    		Int size = tree->size();

    		if (size > 0)
    		{
    			if (other->is_empty(Idx))
    			{
    				other->allocator()->template allocateEmpty<Tree>(Idx);
    			}

    			Tree* other_tree = other->template get<Tree>(Idx);
    			tree->mergeWith(other_tree);
    		}
    	}
    };

    void mergeWith(MyType* other)
    {
    	Dispatcher::dispatchNotEmpty(allocator(), MergeWithFn(), other);
    }

    struct SplitToFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, MyType* other, const Position* indexes)
    	{
    		Int idx   = indexes->value(Idx);
    		Int size  = tree->size();

    		MEMORIA_ASSERT_TRUE(idx >= 0);
    		MEMORIA_ASSERT_TRUE(idx <= size);

    		if (size > 0)
    		{
        		Int size = tree->size();
        		if (size > 0)
        		{
        			Tree* other_tree 	= other->allocator()->template allocateEmpty<Tree>(Idx);

        			tree->splitTo(other_tree, idx);
        		}
    		}
    	}
    };


    Accumulator splitTo(MyType* other, const Position& from)
    {
    	Accumulator result;

    	Position sizes = this->sizes();

    	sums(from, sizes, result);

    	Dispatcher::dispatchNotEmpty(allocator(), SplitToFn(), other, &from);

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

    	Dispatcher::dispatchNotEmpty(allocator(), CopyToFn(), other, copy_from, count, copy_to);
    }


    struct SumsFn {
    	template <Int StreamIdx, typename StreamType>
    	void stream(const StreamType* obj, Int start, Int end, Accumulator& accum)
    	{
    		obj->sums(start, end, std::get<StreamIdx>(accum));
    	}

    	template <Int StreamIdx, typename StreamType>
    	void stream(const StreamType* obj, Int block, Int start, Int end, BigInt& accum)
    	{
    		accum += obj->sum(block, start, end);
    	}

    	template <Int StreamIdx, typename StreamType>
    	void stream(const StreamType* obj, const Position& start, const Position& end, Accumulator& accum)
    	{
    		obj->sums(start[StreamIdx], end[StreamIdx], std::get<StreamIdx>(accum));
    	}

    	template <Int StreamIdx, typename StreamType>
    	void stream(const StreamType* obj, const Position& start, const Position& end, Accumulator& accum, UBigInt streams)
    	{
    		if (streams && (1ull<<StreamIdx))
    		{
    			obj->sums(start[StreamIdx], end[StreamIdx], std::get<StreamIdx>(accum));
    		}
    	}

    	template <Int StreamIdx, typename StreamType>
    	void stream(const StreamType* obj, Accumulator& accum)
    	{
    		obj->sums(std::get<StreamIdx>(accum));
    	}
    };

    void sums(Int start, Int end, Accumulator& sums) const
    {
    	Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), start, end, sums);
    }

    void sums(Int stream, Int start, Int end, Accumulator& sums) const
    {
    	Dispatcher::dispatch(stream, allocator(), SumsFn(), start, end, sums);
    }

    void sums(const Position& start, const Position& end, Accumulator& sums) const
    {
    	Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), start, end, sums);
    }

    void sums(const Position& start, const Position& end, Accumulator& sums, UBigInt active_stereams) const
    {
    	Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), start, end, sums, active_stereams);
    }

    void sum(Int stream, Int block_num, Int start, Int end, BigInt& accum) const
    {
    	Dispatcher::dispatch(stream, allocator(), SumsFn(), block_num, start, end, &accum);
    }

    void sums(Accumulator& sums) const
    {
    	Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), sums);
    }

    Accumulator sums() const
    {
    	Accumulator sums;
    	Dispatcher::dispatchNotEmpty(allocator(), SumsFn(), sums);
    	return sums;
    }


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

    	Dispatcher::dispatchNotEmpty(allocator(), InsertSourceFn(), &src, &pos, &sizes);
    }


    struct AppendSourceFn {
    	template <Int Idx, typename Tree>
    	void stream(Tree* tree, ISource* src, const Position* sizes)
    	{
    		tree->append(src->stream(Idx), sizes->value(Idx));
    	}
    };

    void append(ISource& src, const Position& sizes)
    {
    	initStreamsIfEmpty(sizes);

    	Dispatcher::dispatchNotEmpty(allocator(), AppendSourceFn(), &src, &sizes);
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
    	Dispatcher::dispatchNotEmpty(allocator(), UpdateSourceFn(), src, &pos, &sizes);
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
    	Dispatcher::dispatchNotEmpty(allocator(), ReadToTargetFn(), tgt, &pos, &sizes);
    }


    template <typename Fn, typename... Args>
    Int find(Int stream, Fn&& fn, Args... args) const
    {
    	return Dispatcher::dispatchRtn(stream, allocator(), std::move(fn), args...);
    }

    template <typename Fn, typename... Args>
    void process(Int stream, Fn&& fn, Args... args) const
    {
    	Dispatcher::dispatch(stream, allocator(), std::move(fn), args...);
    }

    template <typename Fn, typename... Args>
    void process(Int stream, Fn&& fn, Args... args)
    {
    	Dispatcher::dispatch(stream, allocator(), std::move(fn), args...);
    }

    template <typename Fn, typename... Args>
    void processAll(Fn&& fn, Args... args) const
    {
    	Dispatcher::dispatchAll(allocator(), std::move(fn), args...);
    }

    template <typename Fn, typename... Args>
    void processAll(Fn&& fn, Args... args)
    {
    	Dispatcher::dispatchAll(allocator(), std::move(fn), args...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    void processStream(Fn&& fn, Args... args) const
    {
    	Dispatcher::template dispatch<StreamIdx>(allocator(), fn, args...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    void processStream(Fn&& fn, Args... args)
    {
    	Dispatcher::template dispatch<StreamIdx>(allocator(), fn, args...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    typename std::remove_reference<Fn>::type::ResultType
    processStreamRtn(Fn&& fn, Args... args) const
    {
    	return Dispatcher::template dispatchRtn<StreamIdx>(allocator(), fn, args...);
    }

    template <Int StreamIdx, typename Fn, typename... Args>
    typename std::remove_reference<Fn>::type::ResultType
    processStreamRtn(Fn&& fn, Args... args)
    {
    	return Dispatcher::template dispatchRtn<StreamIdx>(allocator(), fn, args...);
    }


    struct GenerateDataEventsFn {
    	template <Int Idx, typename Tree>
    	void stream(const Tree* tree, IPageDataEventHandler* handler)
    	{
    		tree->generateDataEvents(handler);
    	}
    };

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);

        allocator()->generateDataEvents(handler);

        Dispatcher::dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);
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

        allocator()->serialize(buf);

        Dispatcher::dispatchNotEmpty(allocator(), SerializeFn(), &buf);
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

        allocator()->deserialize(buf);

        Dispatcher::dispatchNotEmpty(allocator(), DeserializeFn(), &buf);
    }

    static void InitType() {}

    void set_children_count(Int)
    {
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
    	Dispatcher::dispatchNotEmpty(allocator(), DumpFn());
    }
};




}



template <typename Types>
struct TypeHash<bt::LeafNode<Types> > {

	typedef bt::LeafNode<Types> Node;

    static const UInt Value = HashHelper<
    		TypeHash<typename Node::Base>::Value,
    		Node::VERSION,
    		true,
    		Types::Indexes,
    		TypeHash<typename Types::Name>::Value
    >::Value;
};


}

#endif
