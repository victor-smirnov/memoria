
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_VTREE_C_API_HPP
#define MEMORIA_CONTAINERS_VTREE_C_API_HPP

#include <memoria/core/container/container.hpp>

#include <memoria/containers/vector_tree/vtree_names.hpp>

#include <functional>

namespace memoria {


MEMORIA_CONTAINER_PART_BEGIN(memoria::vtree::CtrApiName)

    typedef typename Base::Tree                                                 Tree;
    typedef typename Base::Vec                                                  Vec;

    typedef typename Tree::Iterator                                             TreeIterator;
    typedef typename Vec::Iterator                                              VectorIterator;
    typedef typename Base::Iterator                                             Iterator;

    typedef typename Tree::Types::CtrSizeT                                      CtrSizeT;

    CtrSizeT nodes() {
        return self().tree().nodes();
    }

    CtrSizeT bitmap_size() {
        return self().tree().size();
    }

    void prepare()
    {
        auto& self = this->self();
        auto iter = self.tree().seek(0);
        self.tree().insertZero(iter);
    }

    Iterator root_node()
    {
        auto& self = this->self();

        auto tree_iter  = self.tree().seek(0);
        auto vec_iter   = self.vector().seek(0);

        return Iterator(self, tree_iter, vec_iter);
    }

    Iterator seek(CtrSizeT node_idx)
    {
        auto& self = this->self();

        auto tree_iter   = self.tree().seek(node_idx);
        CtrSizeT data_base = tree_iter.template sumLabel<1>();
        auto vec_iter    = self.vector().seek(data_base);

        return Iterator(self, tree_iter, vec_iter);
    }


    LoudsNode insert(Iterator& pos, Int node_type)
    {
        CtrSizeT node_pos = pos.tree_iter().node().node();

        auto child = self().tree().newNodeAt(pos.tree_iter().node(), std::make_tuple(node_type, 0));

        pos = self().seek(node_pos);

        CtrSizeT data_base = pos.lobBase();
        pos.vector_iter().seek(data_base);

        return child;
    }

//  void remove(Iterator& node)
//  {
//      self().tree().removeLeaf(node.tree_iter());
//  }

    Iterator firstChild(const LoudsNode& node)
    {
        auto& self = this->self();

        auto tree_iter      = self.tree().firstChild(node);
        CtrSizeT data_base  = tree_iter.template sumLabel<1>();
        auto vec_iter       = self.vector().seek(data_base);

        return Iterator(self, tree_iter, vec_iter);
    }


    Iterator lastChild(const LoudsNode& node)
    {
        auto& self = this->self();

        auto tree_iter      = self.tree().lastChild(node);
        CtrSizeT data_base  = tree_iter.template sumLabel<1>();
        auto vec_iter       = self.vector().seek(data_base);

        return Iterator(self, tree_iter, vec_iter);
    }

    Iterator child(const LoudsNode& node, CtrSizeT child_num)
    {
        auto& self = this->self();

        auto tree_iter      = self.tree().child(node, child_num);
        CtrSizeT data_base  = tree_iter.template sumLabel<1>();
        auto vec_iter       = self.vector().seek(data_base);

        return Iterator(self, tree_iter, vec_iter);
    }

    Iterator parent(const LoudsNode& node)
    {
        auto& self = this->self();

        auto tree_iter      = self.tree().parent(node);
        CtrSizeT data_base  = tree_iter.template sumLabel<1>();
        auto vec_iter       = self.vector().seek(data_base);

        return Iterator(self, tree_iter, vec_iter);
    }

    Iterator children(const LoudsNode& node)
    {
        auto& self = this->self();

        auto tree_iter      = self.tree().preFirstChild(node);
        CtrSizeT data_base  = tree_iter.template sumLabel<1>();
        auto vec_iter       = self.vector().seek(data_base);

        return Iterator(self, tree_iter, vec_iter);
    }

    void removeLeaf(const LoudsNode& node)
    {
        auto& self = this->self();

        auto iter = self.seek(node.node());

        CtrSizeT data_base = iter.lobBase();
        CtrSizeT data_size = iter.lobSize();

        auto vec_iter    = self.vector().seek(data_base);
        vec_iter.remove(data_size);

        self.tree().removeLeaf(iter.tree_iter());
    }

MEMORIA_CONTAINER_PART_END

}


#endif
