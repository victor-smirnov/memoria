
#ifndef MEMORIA_PROTOTYPES_BT_INPUT_BUFFER_HPP_

// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#define MEMORIA_PROTOTYPES_BT_INPUT_BUFFER_HPP_

#include <memoria/core/types/types.hpp>

#include <memoria/core/packed/tools/packed_dispatcher.hpp>

namespace memoria 	{
namespace bt 		{

template <typename Position, typename Buffer>
struct InputBufferProvider {
	virtual Position start() const			= 0;
	virtual Position size()	const			= 0;
	virtual Position zero()	const			= 0;

	virtual const Buffer* buffer() const 	= 0;
	virtual void consumed(Position sizes) 	= 0;
	virtual bool isConsumed() 				= 0;

	virtual void nextBuffer() 				= 0;
	virtual bool hasData() const 			= 0;

	InputBufferProvider<Position, Buffer>& me() {return *this;}
};





template <typename Types>
struct CompoundInputBuffer: public PackedAllocator {

	using Base 		= PackedAllocator;
	using MyType 	= CompoundInputBuffer<Types>;

    using Position 		= typename Types::Position;

    using InputBufferStructList 		= typename Types::InputBufferStructList;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
    		Linearize<InputBufferStructList>,
    		0
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList>;

    template <Int StartIdx, Int EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;


    template <typename SubstreamsPath>
    using SubstreamsDispatcher = SubrangeDispatcher<
    		memoria::list_tree::LeafCountInf<InputBufferStructList, SubstreamsPath>::Value,
    		memoria::list_tree::LeafCountSup<InputBufferStructList, SubstreamsPath>::Value
    >;

    template <Int StreamIdx>
    using StreamDispatcher = SubstreamsDispatcher<IntList<StreamIdx>>;

    template <Int StreamIdx>
    using StreamStartIdx = IntValue<
    		memoria::list_tree::LeafCountInf<InputBufferStructList, IntList<StreamIdx>>::Value
    >;



    template <Int Stream, typename SubstreamIdxList>
    using SubstreamsByIdxDispatcher = typename Dispatcher::template SubsetDispatcher<
    		memoria::list_tree::AddToValueList<
    			memoria::list_tree::LeafCount<InputBufferStructList, IntList<Stream>>::Value,
    			SubstreamIdxList
    		>,
    		Stream
    >;


    static const Int Streams                                                    = ListSize<InputBufferStructList>::Value;

    static const Int Substreams                                                 = Dispatcher::Size;

    static const Int SubstreamsStart                                            = Dispatcher::AllocatorIdxStart;
    static const Int SubstreamsEnd                                              = Dispatcher::AllocatorIdxEnd;


    //FIXME: Use SubDispatcher

    template <Int Idx, typename... Args>
    using DispatchRtnFnType = auto(Args...) -> decltype(
            Dispatcher::template dispatch<Idx>(std::declval<Args>()...)
    );

    template <typename... Args>
    using DynDispatchRtnFnType = auto(Args...) -> decltype(
            Dispatcher::template dispatch(std::declval<Args>()...)
    );

    template <Int Idx, typename Fn, typename... T>
    using DispatchRtnType = typename FnTraits<
            DispatchRtnFnType<Idx, PackedAllocator*, Fn, T...>
    >::RtnType;

    template <Int Idx, typename Fn, typename... T>
    using DispatchRtnConstType = typename FnTraits<
            DispatchRtnFnType<Idx, const PackedAllocator*, Fn, T...>
    >::RtnType;

    template <typename Fn, typename... T>
    using DynDispatchRtnType = typename FnTraits<
            DynDispatchRtnFnType<Int, PackedAllocator*, Fn, T...>
    >::RtnType;

    template <typename Fn, typename... T>
    using DynDispatchRtnConstType = typename FnTraits<
            DynDispatchRtnFnType<Int, const PackedAllocator*, Fn, T...>
    >::RtnType;



    template <typename Fn, typename... T>
    using ProcessAllRtnType = typename Dispatcher::template ProcessAllRtnType<Fn, T...>;

    template <typename Fn, typename... T>
    using ProcessAllRtnConstType = typename Dispatcher::template ProcessAllRtnConstType<Fn, T...>;


    template <typename SubstreamsPath, typename Fn, typename... T>
    using ProcessSubstreamsRtnType = typename SubstreamsDispatcher<SubstreamsPath>::template ProcessAllRtnType<Fn, T...>;


    template <typename SubstreamsPath, typename Fn, typename... T>
    using ProcessSubstreamsRtnConstType = typename SubstreamsDispatcher<SubstreamsPath>::template ProcessAllRtnConstType<Fn, T...>;


    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... T>
    using ProcessSubstreamsByIdxRtnType = typename SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::template ProcessAllRtnType<Fn, T...>;

    template <Int Stream, typename SubstreamsIdxList, typename Fn, typename... T>
    using ProcessSubstreamsByIdxRtnConstType = typename SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::template ProcessAllRtnConstType<Fn, T...>;


    template <typename LeafPath>
    using PackedStruct = typename SubstreamsDispatcher<LeafPath>::template StreamTypeT<0>::Type;

    template <Int SubstreamIdx>
    using PackedStructByIdx = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;

    template <typename LeafPath>
    using PackedStructAllocatorIdx = IntValue<SubstreamsDispatcher<LeafPath>::AllocatorIdxStart>;

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

    template <typename LeafPath>
    auto get() -> PackedStruct<LeafPath>*
    {
    	return Base::template get<PackedStruct<LeafPath>>(PackedStructAllocatorIdx<LeafPath>::Value);
    }

    template <typename LeafPath>
    auto get() const -> const PackedStruct<LeafPath>*
    {
    	return Base::template get<PackedStruct<LeafPath>>(PackedStructAllocatorIdx<LeafPath>::Value);
    }


    template <Int SubstreamIdx>
    auto geti() -> PackedStructByIdx<SubstreamIdx>*
    {
    	return Base::template get<PackedStructByIdx<SubstreamIdx>>(SubstreamsStart + SubstreamIdx);
    }

    template <Int SubstreamIdx>
    auto geti() const -> const PackedStructByIdx<SubstreamIdx>*
    {
    	return Base::template get<PackedStructByIdx<SubstreamIdx>>(SubstreamsStart + SubstreamIdx);
    }


    void init(Int block_size)
    {
    	Base::setTopLevelAllocator();
    	Base::init(block_size, Streams);

    	this->layout(-1ull);
    }

    void init(Int block_size, const Position& sizes)
    {
    	Base::setTopLevelAllocator();
    	Base::init(block_size, Streams);

    	this->layout(sizes);
    }

    static Int free_space(Int page_size)
    {
        Int block_size = page_size - sizeof(MyType) + PackedAllocator::my_size();
        Int client_area = PackedAllocator::client_area(block_size, SubstreamsStart + Substreams + 1);

        return client_area;
    }


    PackedAllocator* allocator()
    {
        return this;
    }

    const PackedAllocator* allocator() const
    {
        return this;
    }

    bool is_empty() const
    {
        for (Int c = SubstreamsStart; c < SubstreamsEnd; c++)
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

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Node>
        void stream(Node*, const Position& sizes)
        {
            if (sizes[StreamIdx] > 0)
            {
                size_ += Node::block_size(sizes[StreamIdx]);
            }
        }
    };

public:
    static Int block_size(const Position& sizes)
    {
        BlockSizeFn fn;

        MyType::processSubstreamGroupsStatic(fn, sizes);

        Int client_area = fn.size_;

        return PackedAllocator::block_size(client_area, Streams);
    }


    static Int client_area(Int block_size)
    {
        Int free_space = MyType::free_space(block_size);
        return PackedAllocator::client_area(free_space, Streams);
    }

    Int total_size() const
    {
        return this->allocated();
    }



    struct LayoutFn {
        template <Int AllocatorIdx, Int Idx, typename Stream>
        void stream(Stream*, PackedAllocator* alloc, UBigInt streams)
        {
            if (streams & (1<<Idx))
            {
                if (alloc->is_empty(AllocatorIdx))
                {
                    alloc->template allocateEmpty<Stream>(AllocatorIdx);
                }
            }
        }

        template <Int GroupIdx, Int AllocatorIdx, Int Idx, typename Stream>
        void stream(Stream*, PackedAllocator* alloc, const Position& sizes)
        {
        	if (!alloc->is_empty(AllocatorIdx))
        	{
        		alloc->free(AllocatorIdx);
        	}

        	Int block_size = Stream::block_size(sizes[GroupIdx]);

        	alloc->template allocate<Stream>(AllocatorIdx, block_size);
        }
    };


    void layout(UBigInt streams)
    {
        Dispatcher::dispatchAllStatic(LayoutFn(), this->allocator(), streams);
    }

    void layout(const Position& sizes)
    {
    	this->processSubstreamGroupsStatic(LayoutFn(), this->allocator(), sizes);
    }


    struct ResetFn {
        template <typename Stream>
        void stream(Stream* subs)
        {
            if (subs != nullptr)
            {
            	subs->reset();
            }
        }
    };

    void reset()
    {
    	this->processAll(ResetFn());
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


public:

    struct ObjectSizeFn {
        template <typename Tree>
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



    struct ReindexFn {
    	template <typename Tree>
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
        template <typename Tree>
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
        template <Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, TreeType* other)
        {
            auto allocator 		 = tree->allocator();
            auto other_allocator = other->allocator();

            other_allocator->importBlock(AllocatorIdx, allocator, AllocatorIdx);
        }
    };

    template <typename TreeType>
    void transferDataTo(TreeType* other) const
    {
        Dispatcher::dispatchNotEmpty(allocator(), TransferToFn<TreeType>(), other);
    }

    struct MemUsedFn {
        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, const Position& sizes, Int* mem_used, Int except)
        {
            if (Idx != except)
            {
                Int size = sizes[StreamIdx];

                if (tree != nullptr || size > 0)
                {
                    *mem_used += Tree::packed_block_size(size);
                }
            }
        }
    };

    struct Capacity3Fn {
        template <typename Tree>
        Int stream(const Tree* tree, Int free_mem)
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
        this->processSubstreamGroups(MemUsedFn(), fillment, &mem_used, stream);

        Int client_area = MyType::client_area(this->page_size(), this->is_root());

        return Dispatcher::dispatch(stream, allocator(), Capacity3Fn(), client_area - mem_used);
    }

    Int capacity(const Int* sizes, Int stream) const
    {
        Position psizes;
        for (Int c = 0; c < Streams; c++) psizes[c] = sizes[c];

        return capacity(psizes, stream);
    }


    Int capacity(Int stream) const
    {
        Position sizes = this->sizes();
        return capacity(sizes, stream);
    }


    struct CheckCapacitiesFn {

        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree>
        void stream(const Tree* tree, const Position& sizes, Int* mem_size)
        {
            Int size = sizes[StreamIdx];

            if (tree != nullptr || size > 0)
            {
                *mem_size += Tree::packed_block_size(size);
            }
        }


        template <Int StreamIdx, Int AllocatorIdx, Int Idx, typename Tree, typename Entropy>
        void stream(const Tree* tree, const Entropy& entropy, const Position& sizes, Int* mem_size)
        {
            Int size = sizes[StreamIdx];

            if (tree != nullptr || size > 0)
            {
                *mem_size += Tree::packed_block_size(size);
            }
        }
    };

    bool checkCapacities(const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (Int c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        Int mem_size = 0;

        Dispatcher::dispatchAll(allocator(), CheckCapacitiesFn(), fillment, &mem_size);

        Int free_space      = MyType::free_space(this->page_size(), this->is_root());
        Int client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size;
    }


    template <typename Entropy>
    bool checkCapacities(const Entropy& entropy, const Position& sizes) const
    {
        Position fillment = this->sizes();

        for (Int c = 0; c < Streams; c++)
        {
            fillment[c] += sizes[c];
        }

        Int mem_size = 0;

        this->processSubstreamGroups(CheckCapacitiesFn(), entropy, fillment, &mem_size);

        Int free_space      = MyType::free_space(this->page_size(), this->is_root());
        Int client_area     = PackedAllocator::client_area(free_space, Streams);

        return client_area >= mem_size;
    }



    struct SizeFn {
        template <typename Tree>
        Int stream(const Tree* tree)
        {
            return tree != nullptr ? tree->size() : 0;
        }
    };

    Int size(Int stream) const
    {
        return Dispatcher::dispatch(stream, allocator(), SizeFn());
    }

    template <Int StreamIdx>
    Int streamSize() const
    {
    	return Dispatcher::template dispatch<IntList<StreamIdx>>(allocator(), SizeFn());
    }

    struct SizesFn {
        template <Int StreamIdx, typename Tree>
        void stream(const Tree* tree, Position& pos)
        {
            pos[StreamIdx] = tree != nullptr ? tree->size() : 0;
        }
    };

    Position sizes() const
    {
        Position pos;
        this->processStreamsStart(SizesFn(), pos);
        return pos;
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




    template <typename Fn, typename... Args>
    DynDispatchRtnConstType<Fn, Args...>
    process(Int stream, Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatch(
        		stream,
        		allocator(),
        		std::forward<Fn>(fn),
        		std::forward<Args>(args)...
        );
    }

    template <typename Fn, typename... Args>
    DynDispatchRtnType<Fn, Args...>
    process(Int stream, Fn&& fn, Args&&... args)
    {
        return Dispatcher::dispatch(stream, allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    ProcessAllRtnConstType<Fn, Args...>
    processAll(Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    ProcessAllRtnType<Fn, Args...>
    processAll(Fn&& fn, Args&&... args)
    {
        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamsPath, typename Fn, typename... Args>
    ProcessSubstreamsRtnConstType<SubstreamsPath, Fn, Args...>
    processSubstreams(Fn&& fn, Args&&... args) const
    {
        return SubstreamsDispatcher<SubstreamsPath>::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename SubstreamsPath, typename Fn, typename... Args>
    ProcessSubstreamsRtnType<SubstreamsPath, Fn, Args...>
    processSubstreams(Fn&& fn, Args&&... args)
    {
        return SubstreamsDispatcher<SubstreamsPath>::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }









    template <
    	Int Stream,
    	typename SubstreamsIdxList,
    	typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args) const -> ProcessSubstreamsByIdxRtnConstType<Stream, SubstreamsIdxList, Fn, Args...>
    {
    	return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::dispatchAll(
    			allocator(),
    			std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }

    template <
    	Int Stream,
    	typename SubstreamsIdxList,
    	typename Fn,
        typename... Args
    >
    auto processSubstreamsByIdx(Fn&& fn, Args&&... args) -> ProcessSubstreamsByIdxRtnType<Stream, SubstreamsIdxList, Fn, Args...>
    {
    	return SubstreamsByIdxDispatcher<Stream, SubstreamsIdxList>::dispatchAll(
    			allocator(),
    			std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }





    template <typename SubstreamPath, typename Fn, typename... Args>
    DispatchRtnType<memoria::list_tree::LeafCount<InputBufferStructList, SubstreamPath>::Value, Fn, Args...>
    processStream(Fn&& fn, Args&&... args) const
    {
        const Int StreamIdx = memoria::list_tree::LeafCount<InputBufferStructList, SubstreamPath>::Value;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    DispatchRtnType<memoria::list_tree::LeafCount<InputBufferStructList, SubstreamPath>::Value, Fn, Args...>
    processStream(Fn&& fn, Args&&... args)
    {
        const Int StreamIdx = memoria::list_tree::LeafCount<InputBufferStructList, SubstreamPath>::Value;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    void processSubstreamGroups(Fn&& fn, Args&&... args)
    {
    	using GroupsList = BuildTopLevelLeafSubsets<InputBufferStructList>;

    	GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }

    template <typename Fn, typename... Args>
    void processSubstreamGroups(Fn&& fn, Args&&... args) const
    {
    	using GroupsList = BuildTopLevelLeafSubsets<InputBufferStructList>;

    	GroupDispatcher<Dispatcher, GroupsList>::dispatchGroups(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }

    template <typename Fn, typename... Args>
    static void processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
    	using GroupsList = BuildTopLevelLeafSubsets<InputBufferStructList>;

    	GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }




    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args)
    -> typename Dispatcher::template SubsetDispatcher<
    		StreamsStartSubset<InputBufferStructList>
       >::template ProcessAllRtnConstType<Fn, Args...>
    {
    	using Subset = StreamsStartSubset<InputBufferStructList>;
    	return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }


    template <typename Fn, typename... Args>
    auto processStreamsStart(Fn&& fn, Args&&... args) const
    -> typename Dispatcher::template SubsetDispatcher<
    		StreamsStartSubset<InputBufferStructList>
       >::template ProcessAllRtnConstType<Fn, Args...>
    {
    	using Subset = StreamsStartSubset<InputBufferStructList>;
    	return Dispatcher::template SubsetDispatcher<Subset>::template dispatchAll(
    			allocator(),
    			std::forward<Fn>(fn),
    			std::forward<Args>(args)...
    	);
    }





    struct DumpFn {
        template <typename Tree>
        void stream(Tree* tree)
        {
            tree->dump();
        }
    };


    void dump() const {
        Dispatcher::dispatchNotEmpty(allocator(), DumpFn());
    }
};


template <typename LeafPath, typename Types>
auto get(CompoundInputBuffer<Types>* buf) -> void
{

}


}
}

#endif
