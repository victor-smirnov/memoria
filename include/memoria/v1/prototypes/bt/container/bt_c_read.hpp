
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/v1/prototypes/bt/bt_macros.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {

using memoria::bt::StreamTag;

MEMORIA_CONTAINER_PART_BEGIN(memoria::bt::ReadName)

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
    struct ScanSubstreamBlockFn {

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
    CtrSizeT scan_substream_block(Iterator& iter, Int block, CtrSizeT length, Fn&& fn)
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.idx();

            Int processed = LeafDispatcher::dispatch(iter.leaf(), ScanSubstreamBlockFn<SubstreamPath>(), block, idx, idx + (length - total), std::forward<Fn>(fn));

            if (processed > 0) {
                total += iter.skipFw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(memoria::bt::ReadName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

} //memoria
