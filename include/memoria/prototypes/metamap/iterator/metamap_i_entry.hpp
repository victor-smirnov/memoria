
// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_PROTOTYPES_METAMAP_ITER_ENTRY_HPP
#define _MEMORIA_PROTOTYPES_METAMAP_ITER_ENTRY_HPP

#include <memoria/core/types/types.hpp>

#include <memoria/prototypes/metamap/metamap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/core/packed/map/packed_fse_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrEntryName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Types::Entry                    				Entry;
    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
//    typedef typename Container::Element                                         Element;
    typedef typename Container::Accumulator                                     Accumulator;


    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;


    struct GetEntryFn {
    	using ReturnType = Entry;
    	using ResultType = Entry;

    	template <Int Idx, typename Stream>
    	ReturnType stream(const Stream* stream, Int idx)
    	{
    		MEMORIA_ASSERT_TRUE(stream);
    		return metamap::GetEntry<Entry>(stream, idx);
    	}

    	template <typename Node>
    	ResultType treeNode(const Node* node, Int idx)
    	{
    		return node->template processStreamRtn<0>(*this, idx);
    	}
    };

    Entry getEntry() const
    {
    	auto& self = this->self();
    	return LeafDispatcher::dispatchConstRtn(self.leaf(), GetEntryFn(), self.idx());
    }


    struct SetEntryFn {
        template <Int Idx, typename Stream>
        void stream(Stream* stream, Int idx, const Entry& entry)
        {
            MEMORIA_ASSERT_TRUE(stream);
            metamap::SetEntry(stream, idx, entry);
        }

        template <typename Node>
        void treeNode(Node* node, Int idx, const Entry& value)
        {
            node->template processStream<0>(*this, idx, value);
        }
    };


    void setEntry(const Entry& entry)
    {
        auto& self = this->self();
        self.ctr().updatePageG(self.leaf());
        LeafDispatcher::dispatch(self.leaf(), SetEntryFn(), self.idx(), entry);
    }

    class EntryAccessor {
        MyType& iter_;
    public:
        EntryAccessor(MyType& iter): iter_(iter) {}

        operator Entry() const {
            return iter_.getValue();
        }

        const Entry& operator=(const Entry& value)
        {
            iter_.setEntry(value);
            return value;
        }
    };


    class ConstEntryAccessor {
        const MyType& iter_;
    public:
        ConstEntryAccessor(const MyType& iter): iter_(iter) {}

        operator Entry() const {
            return iter_.getEntry();
        }
    };



    EntryAccessor s_entry() {
    	return EntryAccessor(self());
    }

    Entry entry() const {
    	return self().getEntry();
    }


    void insert(const Key& key, const Value& value)
    {
    	auto& self = this->self();

    	Entry entry;

    	entry.key() 	= key;
    	entry.value()	= value;

    	self.ctr().insertEntry(self, entry);
    }

    void insert(const Entry& entry)
    {
    	auto& self = this->self();
    	self.ctr().insertEntry(self, entry);
    }


    void remove()
    {
    	auto& self = this->self();

    	Accumulator keys;
    	self.ctr().removeMapEntry(self, keys);
    }


MEMORIA_ITERATOR_PART_END

}

#endif
