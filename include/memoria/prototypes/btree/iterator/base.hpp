
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
    typedef typename Base::Container::NodeBaseG                                       NodeBaseG;
    typedef typename Base::Container::Allocator										  Allocator;

private:

    NodeBaseG           page_;
    Int                 key_idx_;

public:
    BTreeIteratorBase(): Base(), page_(NULL), key_idx_(0) {}

    BTreeIteratorBase(ThisType&& other): Base(std::move(other)), page_(std::move(other.page_)), key_idx_(other.key_idx_) {}

    BTreeIteratorBase(const ThisType& other): Base(other), page_(other.page_), key_idx_(other.key_idx_) {}

    void SetupAllocator(Allocator* allocator) {
    	page_.set_allocator(allocator);
    }

    void operator=(ThisType&& other)
    {
        page_       = other.page_;
        key_idx_    = other.key_idx_;

        Base::operator=(std::move(other));
    }

    void operator=(const ThisType& other)
    {
    	page_       = other.page_;
    	key_idx_    = other.key_idx_;

    	Base::operator=(other);
    }

    bool operator==(const MyType& other) const
    {
    	return page_ == other.page_ && key_idx_ == other.key_idx_ && Base::operator==(other);
    }

    Int BuildHash() const {
    	return Base::BuildHash() ^ PtrToHash<const NodeBase*>::hash(page_.page()) ^ key_idx_;
    }

    Int &key_idx() {
        return key_idx_;
    }

    const Int key_idx() const {
        return key_idx_;
    }

    NodeBaseG &page() {
        return page_;
    }

    const NodeBaseG &page() const {
    	return page_;
    }

    bool IsStart()
    {
    	return key_idx() < 0;
    }

    bool IsEnd()
    {
    	return page() != NULL ? key_idx() >= me()->model().GetChildrenCount(page()) : true;
    }

    bool IsEmpty()
    {
    	return page() == NULL || me()->model().GetChildrenCount(page()) == 0;
    }

    bool IsNotEmpty()
    {
    	return !IsEmpty();
    }

MEMORIA_BTREE_ITERATOR_BASE_CLASS_END

} //memoria



#endif
