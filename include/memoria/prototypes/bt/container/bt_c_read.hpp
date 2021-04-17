
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

#include <memoria/prototypes/bt/tools/bt_tools.hpp>
#include <memoria/prototypes/bt/bt_macros.hpp>
#include <memoria/core/container/macros.hpp>


#include <vector>
#include <utility>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(bt::ReadName)

    using typename Base::TreeNodePtr;
    using typename Base::Iterator;
    using typename Base::CtrSizeT;


    template <int32_t StreamIdx>
    struct ReadEntriesFn {

        template <int32_t SubstreamIdx, typename StreamObj, typename Entry>
        auto stream(const StreamObj& obj, Entry&& entry, int32_t idx)
        {
            obj.read(idx, idx + 1, make_fn_with_next([&](int32_t block, auto&& value) {
                entry.put(bt::StreamTag<StreamIdx>(), bt::StreamTag<SubstreamIdx>(), block, value);
            }));
        }

        template <typename Node, typename Fn, typename... Args>
        Int32Result treeNode(const Node& node, Fn&& fn, int32_t from, CtrSizeT to, Args&&... args) noexcept
        {
            int32_t limit = node.size(0);

            if (to < limit) {
                limit = to;
            }

            for (int32_t c = from; c < limit; c++)
            {
                MEMORIA_TRY_VOID(node->template processSubstreams<IntList<StreamIdx>>(*this, fn, c, std::forward<Args>(args)...));

                fn.next();
            }

            return Int32Result::of(limit - from);
        }
    };



    template <int32_t StreamIdx, typename Fn>
    Result<CtrSizeT> ctr_read_entries(Iterator& iter, CtrSizeT length, Fn&& fn) noexcept
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.iter_local_pos();

            MEMORIA_TRY(processed, self().leaf_dispatcher().dispatch(iter.iter_leaf(), ReadEntriesFn<StreamIdx>(), std::forward<Fn>(fn), idx, idx + (length - total)));

            if (processed > 0) {
                total += iter.iter_skip_fw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }

    template <typename SubstreamPath>
    struct ReadSubstreamFn {

        template <int32_t SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj& obj, int32_t from, int32_t to, Fn&& entry) noexcept
        {
            static constexpr int32_t StreamIdx = ListHead<SubstreamPath>::Value;

            obj.read(from, to, make_fn_with_next([&](int32_t block, auto&& value) {
                entry.put(bt::StreamTag<StreamIdx>(), bt::StreamTag<SubstreamIdx>(), value);
            }));
        }

        template <typename NodeSO, typename Fn>
        Int32Result treeNode(const NodeSO& node, int32_t from, CtrSizeT to, Fn&& fn) noexcept
        {
            MEMORIA_TRY(limit, node.size(ListHead<SubstreamPath>::Value));

            if (to < limit) {
                limit = to;
            }

            MEMORIA_TRY_VOID(node.template processStream<SubstreamPath>(*this, from, limit, std::forward<Fn>(fn)));

            return Int32Result::of(limit - from);
        }
    };

    template <typename SubstreamPath, typename Fn>
    Result<CtrSizeT> ctr_read_substream(Iterator& iter, int32_t block, CtrSizeT length, Fn&& fn) noexcept
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.iter_local_pos();

            MEMORIA_TRY(processed, self().leaf_dispatcher().dispatch(iter.iter_leaf(), ReadSubstreamFn<SubstreamPath>(), idx, idx + (length - total), std::forward<Fn>(fn)));

            if (processed > 0) {
                total += iter.iter_skip_fw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }




    template <typename SubstreamPath>
    struct ReadSingleSubstreamFn {

        template <int32_t SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj& obj, int32_t from, int32_t to, int32_t block, Fn&& entry)
        {
            static constexpr int32_t StreamIdx = ListHead<SubstreamPath>::Value;

            obj.read(from, to, make_fn_with_next([&](int32_t block, auto&& value) {
                entry.put(bt::StreamTag<StreamIdx>(), bt::StreamTag<SubstreamIdx>(), value);
            }));
        }

        template <typename Node, typename Fn>
        Int32Result treeNode(const Node& node, int32_t from, CtrSizeT to, int32_t block, Fn&& fn) noexcept
        {
            MEMORIA_TRY(limit, node.size(0));

            if (to < limit) {
                limit = to;
            }

            MEMORIA_TRY_VOID(node.template processStream<SubstreamPath>(*this, from, limit, block, std::forward<Fn>(fn)));

            return Int32Result::of(limit - from);
        }
    };

    template <typename SubstreamPath, typename Fn>
    Result<CtrSizeT> ctr_read_single_substream(Iterator& iter, int32_t block, CtrSizeT length, Fn&& fn) noexcept
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.iter_local_pos();

            MEMORIA_TRY(processed, self().leaf_dispatcher().dispatch(iter.iter_leaf(), ReadSingleSubstreamFn<SubstreamPath>(), block, idx, idx + (length - total), std::forward<Fn>(fn)));

            if (processed > 0) {
                total += iter.iter_skip_fw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }


    template <typename SubstreamPath>
    struct DescribeSingleSubstreamFn {

        template <int32_t SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj& obj, int32_t block, int32_t from, int32_t to, Fn&& entry)
        {
            static constexpr int32_t StreamIdx = ListHead<SubstreamPath>::Value;

            obj.describe(block, from, to, make_fn_with_next([&](int32_t block, auto&& value) {
                return entry.put(bt::StreamTag<StreamIdx>(), bt::StreamTag<SubstreamIdx>(), block, value);
            }));
        }

        template <typename Node, typename Fn>
        Int32Result treeNode(const Node& node, int32_t block, int32_t from, CtrSizeT to, Fn&& fn) noexcept
        {
            MEMORIA_TRY(limit, node->size(0));

            if (to < limit) {
                limit = to;
            }

            MEMORIA_TRY_VOID(node.template processStream<SubstreamPath>(*this, block, from, limit, std::forward<Fn>(fn)));

            return Int32Result::of(limit - from);
        }
    };

    template <typename SubstreamPath, typename Fn>
    Result<CtrSizeT> ctr_describe_single_substream(Iterator& iter, int32_t block, CtrSizeT length, Fn&& fn) noexcept
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.iter_local_pos();

            MEMORIA_TRY(processed, self().leaf_dispatcher().dispatch(iter.iter_leaf(), DescribeSingleSubstreamFn<SubstreamPath>(), block, idx, idx + (length - total), std::forward<Fn>(fn)));

            if (processed > 0) {
                total += iter.iter_skip_fw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }




    template <typename SubstreamPath>
    struct ReadSingleSubstream2Fn {

        template <int32_t SubstreamIdx, typename StreamObj, typename Fn>
        auto stream(const StreamObj& obj, int32_t from, int32_t to, Fn&& entry)
        {
            static constexpr int32_t StreamIdx = ListHead<SubstreamPath>::Value;

            entry.process(bt::StreamTag<StreamIdx>(), bt::StreamTag<SubstreamIdx>(), from, to, obj);
        }

        template <typename Node, typename Fn>
        Int32Result treeNode(const Node& node, int32_t from, CtrSizeT to, Fn&& fn) noexcept
        {
            MEMORIA_TRY(limit, node.size(0));

            if (to < limit) {
                limit = to;
            }

            MEMORIA_TRY_VOID(node.template processStream<SubstreamPath>(*this, from, limit, std::forward<Fn>(fn)));

            return Int32Result::of(limit - from);
        }
    };

    template <typename SubstreamPath, typename Fn>
    Result<CtrSizeT> ctr_read_single_substream2(Iterator& iter, CtrSizeT length, Fn&& fn) noexcept
    {
        CtrSizeT total = 0;

        while (total < length)
        {
            auto idx = iter.iter_local_pos();

            MEMORIA_TRY(processed, self().leaf_dispatcher().dispatch(iter.iter_leaf(), ReadSingleSubstream2Fn<SubstreamPath>(), idx, idx + (length - total), std::forward<Fn>(fn)));

            if (processed > 0) {
                total += iter.iter_skip_fw(processed);
            }
            else {
                break;
            }
        }

        return total;
    }


MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(bt::ReadName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_TYPE
#undef M_PARAMS

}
