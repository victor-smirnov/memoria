
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_LBLTREE_C_API_HPP
#define MEMORIA_CONTAINERS_LBLTREE_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/labeled_tree/ltree_names.hpp>
#include <memoria/containers/labeled_tree/ltree_tools.hpp>

#include <memoria/core/packed/wrappers/louds_tree.hpp>

#include <functional>

namespace memoria {

using louds::LoudsNode;
using louds::LoudsNodeRange;

MEMORIA_CONTAINER_PART_BEGIN(memoria::louds::CtrApiName)

    typedef typename Base::Iterator                                             Iterator;

    typedef typename Base::Types::CtrSizeT                                      CtrSizeT;

    Iterator findNode(const LoudsNode& node)
    {
        return self().seek(node.node());
    }

    Iterator findNode(CtrSizeT node_idx)
    {
        return self().seek(node_idx);
    }

    Iterator rootNode()
    {
        return self().seek(0);
    }

    Iterator parent(const LoudsNode& node)
    {
        return self().select1(node.rank0());
    }


    Iterator children(const LoudsNode& node)
    {
        return self().preFirstChild(node);
    }


    Iterator child(const LoudsNode& node, CtrSizeT child_num)
    {
        Iterator iter = firstChild(node);

        iter.skipFw(child_num);

        return iter;
    }

    Iterator firstChild(const LoudsNode& node)
    {
        Iterator iter = self().select0(node.rank1());

        iter++;

        return iter;
    }

    Iterator preFirstChild(const LoudsNode& node)
    {
        return self().select0(node.rank1());
    }

    Iterator lastChild(const LoudsNode& node)
    {
        Iterator iter   = self().select0(node.rank1() + 1);

        iter--;

        return iter;
    }

    CtrSizeT nodes()
    {
        auto& self = this->self();
        return self.rank1(self.size() - 1);
    }

//  LoudsTree getLoudsSubtree(const LoudsNode& node)
//  {
//      BigInt tree_size = 0;
//
//      this->traverseSubtree(node, [&tree_size](const Iterator& left, BigInt length, Int level) {
//          tree_size += length;
//      });
//
//      LoudsTree tree(tree_size);
//
//      this->traverseSubtree(node, [&tree, this](const Iterator& left, BigInt length, Int level) {
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

        this->traverseSubtree(node, [&count](const Iterator& left, CtrSizeT length, Int level) {
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



    void traverseSubtree(const LoudsNode& node, function<void (const Iterator&, CtrSizeT, Int)> fn)
    {
        Iterator left = self().seek(node.node());
        Iterator right = left;

        traverseSubtree(left, right, fn);
    }

private:
    void traverseSubtree(Iterator& left, Iterator& right, function<void (const Iterator&, CtrSizeT, Int)> fn, Int level = 0)
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

MEMORIA_CONTAINER_PART_END

}


#endif
