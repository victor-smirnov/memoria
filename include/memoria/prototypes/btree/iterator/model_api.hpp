
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef __MEMORIA_PROTOTYPES_BTREE_ITERATOR_MODEL_API_H
#define __MEMORIA_PROTOTYPES_BTREE_ITERATOR_MODEL_API_H

#include <iostream>

#include <memoria/core/types/types.hpp>
#include <memoria/prototypes/btree/names.hpp>




namespace memoria    {

using namespace memoria::btree;


MEMORIA_ITERATOR_PART_BEGIN(memoria::btree::IteratorContainerAPIName)

    typedef typename Base::NodeBase                                             NodeBase;
//    typedef memoria::vapi::Map                                                    	Map;

    typedef typename Base::Container::ApiKeyType                                    ApiKeyType;
    typedef typename Base::Container::ApiValueType                                  ApiValueType;

    typedef typename Base::Container::Page                                          PageType;
    typedef typename Base::Container::ID                                            ID;

    typedef typename Base::Container::Key                                           Key;
    typedef typename Base::Container::Value                                         Value;

    static const Int PAGE_SIZE = Base::Container::Allocator::PAGE_SIZE;

//    virtual memoria::vapi::Container* GetContainer() {
//        return &me_.model();
//    }

    bool Next() {
    	return me_.NextKey();
    }

    bool Prev() {
    	return me_.PrevKey();
    }
    
    void GetValue(BigInt& holder)
    {

    }

//    void GetValue(Key& key, Value& value)
//    {
//    	MEMORIA_TRACER(MyType, me_);
//
//        key   = me_.GetKey(0);
//        value = me_.GetData();
//    }

    memoria::vapi::Page* GetPage() {
        if (me_.page() != NULL) {
            return new PageWrapper<PageType, PAGE_SIZE>(me_.page());
        }
        else {
            return NULL;
        }
    }

    Int GetIndex() {
        return me_.key_idx();
    }

    bool IsFlag(Int flag) {
        switch (flag) {
            case memoria::vapi::Iterator::ITER_EOF:    return me_.IsEof();
            case memoria::vapi::Iterator::ITER_EMPTY:  return me_.IsEmpty();
            case memoria::vapi::Iterator::ITER_BOF:    return me_.IsBof();
            default:                throw MemoriaException(MEMORIA_SOURCE, "Invalig flag name", flag);
        }

    }
    
MEMORIA_ITERATOR_PART_END

}



#endif
