
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_BASE_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_BASE_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/btree/names.hpp>
#include <memoria/prototypes/btree/macros.hpp>

#include <memoria/core/tools/hash.hpp>

namespace memoria    {

using namespace memoria::btree;


MEMORIA_BTREE_ITERATOR_BASE_CLASS_NO_CTOR_BEGIN(BTreeIteratorBase)
public:
    typedef typename Base::Container::NodeBase                                        NodeBase;

private:

    NodeBase*           page_;
    Int                 key_idx_;

public:
    BTreeIteratorBase(MyType &me): Base(me), me_(me), page_(NULL), key_idx_(0) {}

    void setup(const MyType &other) {
        page_       = other.page_;
        key_idx_    = other.key_idx_;

        Base::setup(other);
    }

    bool operator==(const MyType& other) const
    {
    	return page_ == other.page_ && key_idx_ == other.key_idx_ && Base::operator==(other);
    }

    Int BuildHash() const {
    	return Base::BuildHash() ^ PtrToHash<NodeBase*>::hash(page_) ^ key_idx_;
    }

    Int &key_idx() {
        return key_idx_;
    }

    const Int key_idx() const {
        return key_idx_;
    }

    NodeBase* &page() {
        return page_;
    }

    bool IsStart()
    {
    	return key_idx() < 0;
    }

    bool IsEnd()
    {
    	return page() != NULL ? key_idx() >= me_.model().GetChildrenCount(page()) : true;
    }

    bool IsEmpty()
    {
    	return page() == NULL || me_.model().GetChildrenCount(page()) == 0;
    }

    bool IsNotEmpty()
    {
    	return !IsEmpty();
    }


MEMORIA_BTREE_ITERATOR_BASE_CLASS_END

} //memoria



#endif
