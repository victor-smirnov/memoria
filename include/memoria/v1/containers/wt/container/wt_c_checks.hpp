
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/containers/wt/wt_names.hpp>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(wt::CtrChecksName)
public:
    typedef typename Base::Types                                                Types;
    typedef typename Base::Allocator                                            Allocator;

    typedef typename Types::NodeBaseG                                           NodeBaseG;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Allocator::Page::ID                                        ID;

    typedef typename Types::BranchNodeEntry                                     BranchNodeEntry;
    typedef typename Types::Position                                            Position;

    typedef typename Types::BlockUpdateMgr                                      BlockUpdateMgr;

    typedef typename Base::Types::LabelsTuple                                   LabelsTuple;

    static const int32_t Streams                                                = Types::Streams;


    struct CheckContentFn {

        const MyType&   ctr_;
        ID              id_;
        bool            root_;

        bool            errors_     = false;


        CheckContentFn(const MyType& ctr, ID id, bool root):
            ctr_(ctr),
            id_(id),
            root_(root)
        {}


        template <int32_t Idx, typename Stream>
        void stream(const Stream* labels)
        {
            if (labels != nullptr)
            {
                for (int32_t c = 0; c < labels->size(); c++)
                {
                    auto value = labels->value(0, c);

                    if (value == 0 && (c > 0 || !root_))
                    {
                        MMA1_ERROR(ctr_,
                                "Node",
                                id_,
                                "has stream ",
                                Idx,
                                " with zero elements. idx=",
                                c
                        );

                        errors_ = true;
                    }
                }
            }
        }

        template <typename Node>
        void treeNode(const Node* node)
        {
            node->template processStream<IntList<1, 1, 1>>(*this);
        }
    };


    bool ctr_check_content(const NodeBaseG& node) const
    {
        if (!Base::ctr_check_content(node))
        {
            if (node->is_leaf())
            {
                CheckContentFn fn(self(), node->id(), node->is_root());

                self().leaf_dispatcher().dispatch(node, fn);

                return fn.errors_;
            }
            else {
                return false;
            }
        }
        else {
            return true;
        }
    }


MEMORIA_V1_CONTAINER_PART_END

}}
