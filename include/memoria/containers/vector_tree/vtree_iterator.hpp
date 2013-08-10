
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef MEMORIA_CONTAINERS_VTREE_ITERATOR_HPP_
#define MEMORIA_CONTAINERS_VTREE_ITERATOR_HPP_


#include <memoria/containers/vector_tree/vtree_names.hpp>

#include <memoria/core/container/container.hpp>

namespace memoria {

template <typename Types>
class Iter<VTreeIterTypes<Types> >: public IterStart<VTreeIterTypes<Types> >
{
    typedef IterStart<VTreeIterTypes<Types> >               					Base;
    typedef Iter<VTreeIterTypes<Types> >                    					MyType;
    typedef Ctr<VTreeCtrTypes<Types> >                      					ContainerType;

    typedef typename ContainerType::Vec::Iterator            					VectorIterator;
    typedef typename ContainerType::Tree::Iterator         						TreeIterator;

    typedef typename Types::Profile                             				Profile;
    typedef typename Types::Allocator                           				Allocator;
    typedef typename Types::Allocator::CtrShared                				CtrShared;
    typedef typename Types::Allocator::PageG                    				PageG;
    typedef typename PageG::Page::ID                                   			ID;


    ContainerType&      model_;

    TreeIterator   		tree_iter_;
    VectorIterator      vec_iter_;
    bool                exists_;

public:

    Iter(ContainerType &model):
        model_(model), tree_iter_(model.tree()), vec_iter_(model.seq()), exists_(false) {}

    Iter(const MyType& other):
        model_(other.model_), tree_iter_(other.tree_iter_), vec_iter_(other.vec_iter_), exists_(other.exists_) {}

    Iter(ContainerType &model, const TreeIterator& tree_iter, const VectorIterator& seq_iter, bool exists = false):
        model_(model), tree_iter_(tree_iter), vec_iter_(seq_iter), exists_(exists) {}

    Iter(ContainerType &model, const TreeIterator& tree_iter, bool exists = false):
        model_(model), tree_iter_(tree_iter), vec_iter_(model.seq()), exists_(exists) {}

    Iter(ContainerType &model, const VectorIterator& seq_iter, bool exists = false):
        model_(model), tree_iter_(model.tree()), vec_iter_(seq_iter), exists_(exists) {}

    //We have no move constructors for iterator

    MyType* me() {
        return this;
    }

    const MyType* me() const {
        return this;
    }

    MyType& self() {
    	return *this;
    }

    const MyType& self() const {
    	return *this;
    }

    ContainerType& model() {
        return model_;
    }

    const ContainerType& model() const {
        return model_;
    }

    ContainerType& ctr() {
        return model_;
    }

    const ContainerType& ctr() const {
        return model_;
    }

    MyType& operator=(MyType&& other)
    {
        if (this != &other)
        {
            tree_iter_ = std::move(other.tree_iter_);
            vec_iter_ = std::move(other.vec_iter_);

            Base::assign(other);
        }

        return *this;
    }

    MyType& operator=(const MyType& other)
    {
        if (this != &other)
        {
            tree_iter_ = other.tree_iter_;
            vec_iter_ = other.vec_iter_;

            Base::assign(std::move(other));
        }

        return *this;
    }

    MyType& operator=(VectorIterator&& seq_iter)
    {
        vec_iter_ = seq_iter;
        return *this;
    }

    MyType& operator=(TreeIterator&& tree_iter)
    {
        tree_iter_ = tree_iter;
        return *this;
    }

    TreeIterator& tree_iter()
    {
        return tree_iter_;
    }

    const TreeIterator& tree_iter() const
    {
        return tree_iter_;
    }

    VectorIterator& vector_iter()
    {
        return vec_iter_;
    }

    const VectorIterator& vector_iter() const
    {
        return vec_iter_;
    }

    bool exists() const
    {
        return exists_;
    }


    bool operator==(const MyType& other) const
    {
        return isEqual(other);
    }

    bool isEqual(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return vec_iter_ == other.vec_iter_ && tree_iter_ == other.tree_iter_ && Base::isEqual(other);
        }
        else if (other.type() == Base::END)
        {
            return Base::isEnd();
        }
        else if (other.type() == Base::START)
        {
            return Base::isBegin();
        }
        else
        {
            return Base::isEmpty();
        }
    }

    bool operator!=(const MyType& other) const
    {
        return isNotEqual(other);
    }

    bool isNotEqual(const MyType& other) const
    {
        if (other.type() == Base::NORMAL)
        {
            return vec_iter_ != other.vec_iter_ || tree_iter_ != other.tree_iter_ || Base::isNotEqual(other);
        }
        else if (other.type() == Base::END)
        {
            return Base::isNotEnd();
        }
        else if (other.type() == Base::START)
        {
            return !Base::isBegin();
        }
        else
        {
            return !Base::isEmpty();
        }
    }

    bool isEnd() const {
        return vec_iter_.isEnd();
    }

    bool isNotEnd() const {
        return vec_iter_.isNotEnd();
    }

    template <typename T>
    T operator=(const T & value)
    {
        AssignToItem(*this, value);
        return value;
    }

    template <typename T>
    T operator=(T&& value)
    {
        AssignToItem(*this, std::forward<T>(value));
        return value;
    }
};


template <typename Types>
bool operator==(const Iter<VTreeIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isEnd();
}

template <typename Types>
bool operator!=(const Iter<VTreeIterTypes<Types> >& iter, const IterEndMark& mark)
{
    return iter.isNotEnd();
}

}




#endif
