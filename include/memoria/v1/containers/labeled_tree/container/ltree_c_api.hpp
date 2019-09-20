
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

#include <memoria/v1/containers/labeled_tree/ltree_names.hpp>
#include <memoria/v1/containers/labeled_tree/ltree_tools.hpp>

#include <memoria/v1/core/packed/wrappers/louds_tree.hpp>

#include <functional>

namespace memoria {
namespace v1 {

using louds::LoudsNode;
using louds::LoudsNodeRange;

MEMORIA_V1_CONTAINER_PART_BEGIN(louds::CtrApiName)
public:
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;


    auto findNode(const LoudsNode& node)
    {
        return self().seek(node.node());
    }

    auto findNode(CtrSizeT node_idx)
    {
        return self().seek(node_idx);
    }

    auto rootNode()
    {
        return self().seek(0);
    }

    auto parent(const LoudsNode& node)
    {
        return self().select1(node.rank0());
    }


    auto children(const LoudsNode& node)
    {
        return self().preFirstChild(node);
    }


    auto child(const LoudsNode& node, CtrSizeT child_num)
    {
        auto iter = firstChild(node);

        iter->iter_skip_fw(child_num);

        return iter;
    }

    auto firstChild(const LoudsNode& node)
    {
        auto iter = self().select0(node.rank1());

        iter->next();

        return iter;
    }

    auto preFirstChild(const LoudsNode& node)
    {
        return self().select0(node.rank1());
    }

    auto lastChild(const LoudsNode& node)
    {
        auto iter = self().select0(node.rank1() + 1);

        iter->prev();

        return iter;
    }

    CtrSizeT nodes()
    {
        auto& self = this->self();
        return self.rank1(self.size() - 1);
    }

//  LoudsTree getLoudsSubtree(const LoudsNode& node)
//  {
//      int64_t tree_size = 0;
//
//      this->traverseSubtree(node, [&tree_size](const Iterator& left, int64_t length, int32_t level) {
//          tree_size += length;
//      });
//
//      LoudsTree tree(tree_size);
//
//      this->traverseSubtree(node, [&tree, this](const Iterator& left, int64_t length, int32_t level) {
//          if (length >= 0)
//          {
//              if (length > 0)
//              {
//                  auto src = left.source(length - 1);
//                  tree.append(src);
//              }
//
//              tree.appendUDS(0);
//          }
//      });
//
//      tree.reindex();
//
//      return tree;
//  }

    CtrSizeT getSubtreeSize(const LoudsNode& node)
    {
        CtrSizeT count = 0;

        this->traverseSubtree(node, [&count](const Iterator& left, CtrSizeT length, int32_t level) {
            if (level == 0)
            {
                count += left.rank1(length - 1);
            }
            else {
                count += left.rank1(length);
            }
        });

        return count;
    }



    void traverseSubtree(const LoudsNode& node, function<void (const Iterator&, CtrSizeT, int32_t)> fn)
    {
        Iterator left = self().seek(node.node());
        Iterator right = left;

        traverseSubtree(left, right, fn);
    }

private:
    void traverseSubtree(Iterator& left, Iterator& right, function<void (const Iterator&, CtrSizeT, int32_t)> fn, int32_t level = 0)
    {
        CtrSizeT length = right.nodeIdx() - left.nodeIdx() + 1;

        fn(left, length + 1, level);

        bool left_leaf      = left.isLeaf();
        bool right_leaf     = right.isLeaf();

        if (!left_leaf || !right_leaf || !is_end(left, right))
        {
            if (left_leaf)
            {
                left.select1Fw(1);
            }

            if (right_leaf)
            {
                right.select1Bw(1);
            }

            if (!left.isEof())
            {
                left.firstChild();
                right.lastChild();

                traverseSubtree(left, right, fn, level + 1);
            }
        }
    }

    bool is_end(Iterator& left, Iterator& right)
    {
        return left.nodeIdx() >= right.nodeIdx();
    }

MEMORIA_V1_CONTAINER_PART_END

}}
