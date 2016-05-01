
// Copyright 2011 Victor Smirnov
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

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <vector>
#include <utility>

namespace memoria {
namespace v1 {

using v1::bt::StreamTag;

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bt::ReadName)

    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Base::ID                                                   ID;
    
    typedef typename Types::NodeBase                                            NodeBase;
    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    using NodeDispatcher    = typename Types::Pages::NodeDispatcher;
    using LeafDispatcher    = typename Types::Pages::LeafDispatcher;
    using BranchDispatcher  = typename Types::Pages::BranchDispatcher;

    typedef typename Base::Metadata                                             Metadata;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;
    typedef typename Types::CtrSizeT                                            CtrSizeT;


    class PopulateIOBufferStatus {
    	Int processed_;
    	bool full_;
    public:
    	PopulateIOBufferStatus(Int processed, bool full):
    		processed_(processed), full_(full)
    	{}

    	Int processed() const {return processed_;}
    	bool is_full() const {return full_;}
    };


    template <Int StreamIdx>
    struct PopulateIOBufferFn {

    	bool proceed_ = true;

    	template <typename StructDescr>
    	using ReadStateFn = HasType<typename StructDescr::Type::ReadState>;

    	template <Int SubstreamIdx, typename StreamObj, typename ReadState, typename IOBuffer>
    	void stream(const StreamObj* obj, ReadState& state, IOBuffer& buffer)
    	{
    		proceed_ = proceed_ && obj->readTo(std::get<SubstreamIdx>(state), buffer);
    	}

    	template <Int SubstreamIdx, typename StreamObj, typename ReadState>
    	void stream(const StreamObj* obj, ReadState& state, Int idx)
    	{
    		std::get<SubstreamIdx>(state) = obj->positions(idx);
    	}


        template <typename NodeType, typename IOBuffer>
        PopulateIOBufferStatus treeNode(const LeafNode<NodeType>* node, IOBuffer&& buffer, Int from, CtrSizeT to)
        {
            using Node = LeafNode<NodeType>;

            using ReadStatesTuple = AsTuple<typename Node::template MapStreamStructs<StreamIdx, ReadStateFn>>;

        	Int limit = node->size(0);

            if (to < limit) {
                limit = to;
            }

            ReadStatesTuple read_state;

            node->template processSubstreams<IntList<StreamIdx>>(*this, read_state, from);

            for (Int c = from; c < limit; c++)
            {
            	size_t current_pos = buffer.pos();
            	node->template processSubstreams<IntList<StreamIdx>>(*this, read_state, buffer);

            	if (!proceed_)
            	{
            		buffer.pos(current_pos);
            		return PopulateIOBufferStatus(c - from, true);
            	}
            }

            return PopulateIOBufferStatus(limit - from, false);
        }
    };



    template <Int StreamIdx, typename IOBuffer>
    CtrSizeT buffered_read(Iterator& iter, CtrSizeT length, IOBuffer& io_buffer, bt::BufferConsumer<IOBuffer>& consumer)
    {
        CtrSizeT total = 0;
        io_buffer.rewind();

        Int entries = 0;
        Int committed_buffer_position_ = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            auto result = LeafDispatcher::dispatch(iter.leaf(), PopulateIOBufferFn<StreamIdx>(), io_buffer, idx, idx + (length - total));

            if (result.processed() > 0)
            {
            	entries += result.processed();

                if (result.is_full())
                {
                	io_buffer.rewind();
                	Int consumed = consumer.process(io_buffer, entries);
                	io_buffer.rewind();
                	entries = 0;

                	if (consumed >= 0)
                	{
                		total += iter.skipFw(result.processed());
                		committed_buffer_position_ = 0;
                	}
                	else {
                		CtrSizeT distance = committed_buffer_position_ + (-consumed);
                		if (distance >= 0)
                		{
                			total += iter.skipFw(distance);
                		}
                		else {
                			total -= iter.skipBw(-distance);
                		}

                		break;
                	}
                }
                else {
                	total += iter.skipFw(result.processed());
                	committed_buffer_position_ += result.processed();
                }
            }
            else if (result.is_full())
            {
            	io_buffer.rewind();
            	Int consumed = consumer.process(io_buffer, entries);
            	io_buffer.rewind();
            	entries = 0;

            	if (consumed < 0)
            	{
            		CtrSizeT distance = committed_buffer_position_ + (-consumed);
            		if (distance >= 0)
            		{
            			total += iter.skipFw(distance);
            		}
            		else {
            			total -= iter.skipBw(-distance);
            		}

            		break;
            	}
            }
            else {
            	break;
            }
        }

        if (entries > 0)
        {
        	io_buffer.rewind();
        	Int consumed = consumer.process(io_buffer, entries);

        	if (consumed < 0)
        	{
        		CtrSizeT distance = committed_buffer_position_ + (-consumed);
        		if (distance >= 0)
        		{
        			total += iter.skipFw(distance);
        		}
        		else {
        			total -= iter.skipBw(-distance);
        		}
        	}
        }

        return total;
    }



    template <Int StreamIdx, typename IOBuffer>
    CtrSizeT populate_buffer(Iterator& iter, CtrSizeT length, IOBuffer& io_buffer)
    {
        CtrSizeT total = 0;
        io_buffer.rewind();

        while (total < length)
        {
            auto idx = iter.idx();

            auto result = LeafDispatcher::dispatch(iter.leaf(), PopulateIOBufferFn<StreamIdx>(), io_buffer, idx, idx + (length - total));

            if (result.processed() > 0)
            {
            	total += iter.skipFw(result.processed());

                if (result.is_full())
                {
                	break;
                }
            }
            else {
            	break;
            }
        }

        return total;
    }







    template <Int StreamIdx>
    struct ReadEntriesFn {

        template <Int SubstreamIdx, typename StreamObj, typename Entry>
        auto stream(const StreamObj* obj, Entry&& entry, Int idx)
        {
            obj->read(idx, idx + 1, make_fn_with_next([&](Int block, auto&& value) {
                entry.put(StreamTag<StreamIdx>(), StreamTag<SubstreamIdx>(), block, value);
            }));
        }

        template <typename Node, typename Fn, typename... Args>
        Int treeNode(const Node* node, Fn&& fn, Int from, CtrSizeT to, Args&&... args)
        {
            Int limit = node->size(0);

            if (to < limit) {
                limit = to;
            }

            for (Int c = from; c < limit; c++)
            {
                node->template processSubstreams<IntList<StreamIdx>>(*this, fn, c, std::forward<Args>(args)...);

                fn.next();
            }

            return limit - from;
        }
    };



    template <Int StreamIdx, typename Fn>
    CtrSizeT read_entries(Iterator& iter, CtrSizeT length, Fn&& fn)
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadEntriesFn<StreamIdx>(), std::forward<Fn>(fn), idx, idx + (length - total));

            if (processed > 0) {
                total += iter.skipFw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }

    template <typename SubstreamPath>
    struct ReadSubstreamFn {

        template <Int SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj* obj, Int from, Int to, Fn&& entry)
        {
            static constexpr Int StreamIdx = ListHead<SubstreamPath>::Value;

            obj->read(from, to, make_fn_with_next([&](Int block, auto&& value) {
                entry.put(StreamTag<StreamIdx>(), StreamTag<SubstreamIdx>(), value);
            }));
        }

        template <typename Node, typename Fn>
        Int treeNode(const Node* node, Int from, CtrSizeT to, Fn&& fn)
        {
            Int limit = node->size(ListHead<SubstreamPath>::Value);

            if (to < limit) {
                limit = to;
            }

            node->template processStream<SubstreamPath>(*this, from, limit, std::forward<Fn>(fn));

            return limit - from;
        }
    };

    template <typename SubstreamPath, typename Fn>
    CtrSizeT read_substream(Iterator& iter, Int block, CtrSizeT length, Fn&& fn)
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadSubstreamFn<SubstreamPath>(), idx, idx + (length - total), std::forward<Fn>(fn));

            if (processed > 0) {
                total += iter.skipFw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }




    template <typename SubstreamPath>
    struct ReadSingleSubstreamFn {

        template <Int SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj* obj, Int from, Int to, Int block, Fn&& entry)
        {
            static constexpr Int StreamIdx = ListHead<SubstreamPath>::Value;

            obj->read(from, to, make_fn_with_next([&](Int block, auto&& value) {
                entry.put(StreamTag<StreamIdx>(), StreamTag<SubstreamIdx>(), value);
            }));
        }

        template <typename Node, typename Fn>
        Int treeNode(const Node* node, Int from, CtrSizeT to, Int block, Fn&& fn)
        {
            Int limit = node->size(0);

            if (to < limit) {
                limit = to;
            }

            node->template processStream<SubstreamPath>(*this, from, limit, block, std::forward<Fn>(fn));

            return limit - from;
        }
    };

    template <typename SubstreamPath, typename Fn>
    CtrSizeT read_single_substream(Iterator& iter, Int block, CtrSizeT length, Fn&& fn)
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadSingleSubstreamFn<SubstreamPath>(), block, idx, idx + (length - total), std::forward<Fn>(fn));

            if (processed > 0) {
                total += iter.skipFw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }


    template <typename SubstreamPath>
    struct DescribeSingleSubstreamFn {

        template <Int SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj* obj, Int block, Int from, Int to, Fn&& entry)
        {
            static constexpr Int StreamIdx = ListHead<SubstreamPath>::Value;

            obj->describe(block, from, to, make_fn_with_next([&](Int block, auto&& value) {
                return entry.put(StreamTag<StreamIdx>(), StreamTag<SubstreamIdx>(), block, value);
            }));
        }

        template <typename Node, typename Fn>
        Int treeNode(const Node* node, Int block, Int from, CtrSizeT to, Fn&& fn)
        {
            Int limit = node->size(0);

            if (to < limit) {
                limit = to;
            }

            node->template processStream<SubstreamPath>(*this, block, from, limit, std::forward<Fn>(fn));

            return limit - from;
        }
    };

    template <typename SubstreamPath, typename Fn>
    CtrSizeT describe_single_substream(Iterator& iter, Int block, CtrSizeT length, Fn&& fn)
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            Int processed = LeafDispatcher::dispatch(iter.leaf(), DescribeSingleSubstreamFn<SubstreamPath>(), block, idx, idx + (length - total), std::forward<Fn>(fn));

            if (processed > 0) {
                total += iter.skipFw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }




    template <typename SubstreamPath>
    struct ReadSingleSubstream2Fn {

        template <Int SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj* obj, Int from, Int to, Fn&& entry)
        {
            static constexpr Int StreamIdx = ListHead<SubstreamPath>::Value;

            entry.process(StreamTag<StreamIdx>(), StreamTag<SubstreamIdx>(), from, to, *obj);
        }

        template <typename Node, typename Fn>
        Int treeNode(const Node* node, Int from, CtrSizeT to, Fn&& fn)
        {
            Int limit = node->size(0);

            if (to < limit) {
                limit = to;
            }

            node->template processStream<SubstreamPath>(*this, from, limit, std::forward<Fn>(fn));

            return limit - from;
        }
    };

    template <typename SubstreamPath, typename Fn>
    CtrSizeT read_single_substream2(Iterator& iter, CtrSizeT length, Fn&& fn)
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            Int processed = LeafDispatcher::dispatch(iter.leaf(), ReadSingleSubstream2Fn<SubstreamPath>(), idx, idx + (length - total), std::forward<Fn>(fn));

            if (processed > 0) {
                total += iter.skipFw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bt::ReadName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}}
