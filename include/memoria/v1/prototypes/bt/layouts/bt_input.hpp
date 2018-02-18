
// Copyright 2015 Victor Smirnov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#pragma once

#include <memoria/v1/core/config.hpp>

#include <memoria/v1/prototypes/bt/tools/bt_tools_substreamgroup_dispatcher.hpp>

#include <memoria/v1/core/packed/tools/packed_dispatcher.hpp>
#include <memoria/v1/core/packed/tools/packed_allocator.hpp>

#include <memoria/v1/core/packed/sseq/packed_fse_searchable_seq.hpp>
#include <memoria/v1/core/packed/buffer/packed_rle_sequence_input_buffer.hpp>

#include <memoria/v1/core/tools/bitmap.hpp>
#include <memoria/v1/core/types/algo/for_each.hpp>
#include <memoria/v1/core/packed/tools/packed_malloc.hpp>
#include <memoria/v1/core/exceptions/exceptions.hpp>



#include <cstdlib>
#include <tuple>

namespace memoria {
namespace v1 {
namespace bt {

template <typename Position, typename Buffer>
struct InputBufferProvider {
    virtual Position start() const          = 0;
    virtual Position size() const           = 0;
    virtual Position zero() const           = 0;

    virtual const Buffer* buffer() const    = 0;
    virtual void consumed(Position sizes)   = 0;
    virtual bool isConsumed()               = 0;

    virtual void nextBuffer()               = 0;
    virtual bool hasData() const            = 0;

    InputBufferProvider<Position, Buffer>& me() {return *this;}
};





template <
    int32_t InputBufferStreamIdx,
    typename SubstreamsStructList
>
class StreamInputBuffer: public PackedAllocator {
public:
    using Base = PackedAllocator;
    using MyType = StreamInputBuffer<InputBufferStreamIdx, SubstreamsStructList>;

    using StreamDispatcherStructList = typename PackedDispatchersListBuilder<
            Linearize<SubstreamsStructList>
    >::Type;

    using Dispatcher = PackedDispatcher<StreamDispatcherStructList>;

    template <int32_t StartIdx, int32_t EndIdx>
    using SubrangeDispatcher = typename Dispatcher::template SubrangeDispatcher<StartIdx, EndIdx>;



    static const int32_t Streams = 1;

    static const int32_t Substreams                                                 = Dispatcher::Size;

    static const int32_t SubstreamsStart                                            = Dispatcher::AllocatorIdxStart;
    static const int32_t SubstreamsEnd                                              = Dispatcher::AllocatorIdxEnd;

    using SizesT = core::StaticVector<int32_t, Substreams>;

    template <typename T>
    using SizeTMapper = WithType<typename T::SizesT>;

    using BufferSizesT = AsTuple<typename MapTL<
            Linearize<SubstreamsStructList>,
            SizeTMapper
    >::Type>;

    template <int32_t SubstreamIdx>
    using StreamTypeT = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;


    template <typename StructDescr>
    using BuildAppendStateFn = HasType<typename StructDescr::Type::AppendState>;

    template <template <typename> class MapFn>
    using MapStreamStructs = typename Dispatcher::template ForAllStructs<MapFn>;

    using AppendState = AsTuple<MapStreamStructs<BuildAppendStateFn>>;

private:
    struct InitFn {
        int32_t block_size(int32_t items_number) const
        {
            return MyType::block_size(items_number);
        }

        int32_t max_elements(int32_t block_size)
        {
            return block_size;
        }
    };

    struct LayoutFn {
        template <int32_t AllocatorIdx, int32_t Idx, typename Stream>
        void stream(Stream*, PackedAllocator* alloc, int32_t capacity)
        {
            using BSizesT = typename Stream::SizesT;
            Stream* buffer = alloc->template allocateSpace<Stream>(AllocatorIdx, Stream::block_size(BSizesT(capacity)));

            buffer->init(BSizesT(capacity));
        }

        template <int32_t AllocatorIdx, int32_t Idx, typename Stream>
        void stream(Stream*, PackedAllocator* alloc, const BufferSizesT& capacities)
        {
            Stream* buffer = alloc->template allocateSpace<Stream>(AllocatorIdx, Stream::block_size(std::get<Idx>(capacities)));
            buffer->init(std::get<Idx>(capacities));
        }
    };


public:

    void init(int32_t buffer_block_size, int32_t capacity)
    {
        Base::init(buffer_block_size, Substreams);

        Dispatcher::dispatchAllStatic(LayoutFn(), allocator(), capacity);
    }

    void init(int32_t buffer_block_size, const BufferSizesT& capacities)
    {
        Base::init(buffer_block_size, Substreams);

        Dispatcher::dispatchAllStatic(LayoutFn(), allocator(), capacities);
    }


    struct DataCapacityFn {

        template <int32_t Idx, typename Stream>
        void stream(Stream* buf, BufferSizesT& capacities)
        {
            std::get<Idx>(capacities) = buf->data_capacity();
        }
    };


    BufferSizesT data_capacity() const
    {
        BufferSizesT capacities;

        Dispatcher::dispatchAll(allocator(), DataCapacityFn(), capacities);

        return capacities;
    }

    struct CopyToFn {

        template <int32_t Idx, typename Stream>
        void stream(Stream* buf, MyType* other)
        {
            if (buf) {
                buf->copyTo(other->template substream_by_idx<Idx>());
            }
        }
    };


    void copyTo(MyType* other) const
    {
        Dispatcher::dispatchAll(allocator(), CopyToFn(), other);
    }


    static int32_t free_space(int32_t page_size)
    {
        int32_t block_size = page_size - sizeof(MyType) + PackedAllocator::my_size();
        int32_t client_area = PackedAllocator::client_area(block_size, SubstreamsStart + Substreams + 1);

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
        for (int32_t c = SubstreamsStart; c < SubstreamsEnd; c++)
        {
            if (!allocator()->is_empty(c)) {
                return false;
            }
        }

        return true;
    }


private:
    struct BlockSizeFn {
        int32_t size_ = 0;

        template <int32_t Idx, typename Node>
        void stream(Node*, const SizesT& sizes)
        {
            size_ += PackedAllocatable::roundUpBytesToAlignmentBlocks(Node::block_size(sizes[Idx]));
        }

        template <int32_t Idx, typename Node>
        void stream(Node*, const BufferSizesT& sizes)
        {
            size_ += PackedAllocatable::roundUpBytesToAlignmentBlocks(
                    Node::block_size(
                            std::get<Idx>(sizes)
                    )
            );
        }
    };

    struct BlockSizeForBufferFn {
        int32_t client_area_ = 0;

        template <int32_t Idx, typename Stream, typename EntryBuffer>
        void stream(Stream*, int32_t size, EntryBuffer&& buffer)
        {
            auto sizes = Stream::calculate_size(size, [&](int32_t block, int32_t idx){
                return std::get<Idx>(buffer[idx])[block];
            });

            client_area_ += PackedAllocatable::roundUpBytesToAlignmentBlocks(Stream::block_size(sizes));
        }
    };


    struct ComputeBufferSizeFn {
        BufferSizesT buffer_size_;

        template <int32_t Idx, typename Stream, typename EntryBuffer>
        auto stream(Stream*, int32_t size, EntryBuffer&& buffer)
        {
            std::get<Idx>(buffer_size_) = Stream::calculate_size(size, [&](int32_t block, int32_t idx) {
                return std::get<Idx>(buffer[idx])[block];
            });
        }
    };


public:
    static int32_t block_size(int32_t size) {
        return block_size(SizesT(size));
    }

    static int32_t block_size(const SizesT& sizes)
    {
        BlockSizeFn fn;

        MyType::processSubstreamGroupsStatic(fn, sizes);

        int32_t client_area = fn.size_;

        return PackedAllocator::block_size(client_area, SubstreamsStart + Substreams + 1);
    }

    static int32_t block_size(const BufferSizesT& sizes)
    {
        BlockSizeFn fn;

        MyType::processSubstreamGroupsStatic(fn, sizes);

        int32_t client_area = fn.size_;

        return PackedAllocator::block_size(client_area, SubstreamsStart + Substreams + 1);
    }


    template <typename EntryBuffer>
    static BufferSizesT compute_buffer_sizes_for(int32_t size, EntryBuffer&& buffer)
    {
        ComputeBufferSizeFn fn;

        MyType::processSubstreamGroupsStatic(fn, size, std::forward<EntryBuffer>(buffer));

        return fn.buffer_size_;
    }

    template <typename EntryBuffer>
    static int32_t compute_bttl_buffer_sizes_for(int32_t size, EntryBuffer&& buffer)
    {
        ComputeBufferSizeFn fn;

        MyType::processSubstreamGroupsStatic(fn, size, std::forward<EntryBuffer>(buffer));

        return fn.buffer_size_;
    }


    template <typename EntryBuffer>
    static int32_t block_size_for_buffer(int32_t size, EntryBuffer&& buffer)
    {
        BufferSizesT sizes = compute_buffer_sizes_for(size, std::forward<EntryBuffer>(buffer));
        return block_size(sizes);
    }

    struct HasCapacityForFn {
        bool value_ = true;

        template <int32_t Idx, typename Stream, typename EntryBuffer>
        auto stream(const Stream* obj, EntryBuffer&& buffer)
        {
            value_ = value_ && obj->has_capacity_for(std::get<Idx>(buffer));
        }
    };


    bool has_capacity_for(const BufferSizesT& sizes) const
    {
        HasCapacityForFn fn;
        processAll(fn, sizes);
        return fn.value_;
    }


    struct HasBTTLCapacityForFn
    {
        template <int32_t Idx, typename Stream, typename SizesBuffer>
        auto stream(const Stream* obj, SizesBuffer&& sizes_buffer, int start, int length)
        {
            return obj->has_capacity_for(sizes_buffer, start, length);
        }
    };


    template <typename SizesBufferT>
    bool has_bttl_capacity_for(const SizesBufferT& sizes_buffer, int start, int length) const
    {
        return processLastSubstream(HasBTTLCapacityForFn(), sizes_buffer, start, length);
    }

    int32_t total_size() const
    {
        return allocator()->allocated();
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

    struct ResetFn {
        template <typename Tree>
        void stream(Tree* tree)
        {
            tree->reset();
        }
    };

    void reset()
    {
        Dispatcher::dispatchNotEmpty(allocator(), ResetFn());
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


    struct SizeFn {
        template <typename Tree>
        int32_t stream(const Tree* tree)
        {
            return tree != nullptr ? tree->size() : 0;
        }

        template <int32_t Idx, typename Tree>
        void stream(const Tree* tree, SizesT& sizes)
        {
            sizes[Idx] = tree != nullptr ? tree->size() : 0;
        }
    };


    int32_t size() const
    {
        return this->processStream<IntList<0>>(SizeFn());
    }

    SizesT sizes() const
    {
        SizesT sizes0;
        Dispatcher::dispatchAll(allocator(), SizeFn(), sizes0);
        return sizes0;
    }

    bool isEmpty() const
    {
        return size() == 0;
    }



    struct EmplaceBackFn {

        bool status_ = false;

        template <int32_t Idx, typename Value, int32_t Indexes, PkdSearchType SearchType>
        void stream(PackedSizedStruct<Value, Indexes, SearchType>* stream, int32_t symbol, uint64_t length)
        {
            stream->insert(0, length);
        }

        template <int32_t Idx, typename SeqTypes>
        void stream(PkdRLESeqInputBuffer<SeqTypes>* stream, int32_t symbol, uint64_t length)
        {
            status_ = stream->emplace_back(symbol, length);
        }
    };


    bool emplace_back_symbols_run(int32_t symbol, uint64_t length)
    {
        EmplaceBackFn fn;
        Dispatcher::dispatchAll(allocator(), fn, symbol, length);
        return fn.status_;
    }





    struct AppendLastSubstreamFn {

        template <int32_t Idx, typename StreamObj, typename SizesBuffer>
        void stream(StreamObj* stream, SizesBuffer&& data, int32_t start, int32_t length)
        {
            stream->append(length, [&](int32_t block, int32_t idx) -> const auto& {
                return data[start + idx];
            });
        }
    };

    template <typename... Args>
    auto append_last_substream(Args&&... args)
    {
        return processLastSubstream(AppendLastSubstreamFn(), std::forward<Args>(args)...);
    }


    struct AppendEntryFromIOBufferFn {
        bool proceed_ = true;

        template <int32_t Idx, typename StreamObj, typename IOBuffer>
        void stream(StreamObj* stream, AppendState& state, IOBuffer& buffer)
        {
            proceed_ = proceed_ && stream->append_entry_from_iobuffer(std::get<Idx>(state), buffer);
        }
    };



    template <typename AppendState, typename IOBuffer>
    bool append_entry_from_iobuffer(AppendState& state, IOBuffer& buffer)
    {
        AppendEntryFromIOBufferFn fn;
        SubrangeDispatcher<0, Substreams>::dispatchAll(allocator(), fn, state, buffer);
        return fn.proceed_;
    }


    template <typename AppendState, typename IOBuffer>
    bool append_bttl_entry_from_iobuffer(AppendState& state, IOBuffer& buffer)
    {
        AppendEntryFromIOBufferFn fn;
        SubrangeDispatcher<0, Substreams - 1>::dispatchAll(allocator(), fn, state, buffer);
        return fn.proceed_;
    }


    struct AppendStateFn {
        AppendState state_;

        template <int32_t Idx, typename StreamObj>
        void stream(StreamObj* stream)
        {
            std::get<Idx>(state_) = stream->append_state();
        }
    };


    AppendState append_state()
    {
        AppendStateFn fn;
        Dispatcher::dispatchAll(allocator(), fn);
        return fn.state_;
    }

    struct RestoreAppendStateFn {
        template <int32_t Idx, typename StreamObj>
        void stream(StreamObj* stream, const AppendState& state)
        {
            stream->restore(std::get<Idx>(state));
        }
    };

    void restore_append_state(const AppendState& state)
    {
        Dispatcher::dispatchAll(allocator(), RestoreAppendStateFn(), state);
    }



    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args) const
    {
        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }


    template <typename Fn, typename... Args>
    auto processAll(Fn&& fn, Args&&... args)
    {
        return Dispatcher::dispatchAll(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <int32_t SubstreamIdx>
    auto substream_by_idx()
    {
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }

    template <int32_t SubstreamIdx>
    auto substream_by_idx() const
    {
        using T = typename Dispatcher::template StreamTypeT<SubstreamIdx>::Type;
        return this->allocator()->template get<T>(SubstreamIdx + SubstreamsStart);
    }


    template <typename Fn, typename... Args>
    auto processLastSubstream(Fn&& fn, Args&&... args) const
    {
        const int32_t StreamIdx = Substreams - 1;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    auto processLastSubstream(Fn&& fn, Args&&... args)
    {
        const int32_t StreamIdx = Substreams - 1;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }



    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args) const
    {
        const int32_t StreamIdx = v1::list_tree::LeafCount<SubstreamsStructList, SubstreamPath>;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename SubstreamPath, typename Fn, typename... Args>
    auto processStream(Fn&& fn, Args&&... args)
    {
        const int32_t StreamIdx = v1::list_tree::LeafCount<SubstreamsStructList, SubstreamPath>;
        return Dispatcher::template dispatch<StreamIdx>(allocator(), std::forward<Fn>(fn), std::forward<Args>(args)...);
    }

    template <typename Fn, typename... Args>
    static auto processSubstreamGroupsStatic(Fn&& fn, Args&&... args)
    {
        using GroupsList = BuildTopLevelLeafSubsets<SubstreamsStructList>;

        return GroupDispatcher<Dispatcher, GroupsList>::dispatchGroupsStatic(
                std::forward<Fn>(fn),
                std::forward<Args>(args)...
        );
    }



    struct GenerateDataEventsFn {
        template <int32_t Idx, typename Tree>
        void stream(const Tree* tree, IPageDataEventHandler* handler)
        {
            tree->generateDataEvents(handler);
        }
    };

    void generateDataEvents(IPageDataEventHandler* handler) const
    {
        Base::generateDataEvents(handler);
        Dispatcher::dispatchNotEmpty(allocator(), GenerateDataEventsFn(), handler);
    }

    void dump(std::ostream& out = std::cout) {
        TextPageDumper dumper(out);
        generateDataEvents(&dumper);
    }
};



}
}}
