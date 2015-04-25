
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

#include <memoria/core/packed/map/packed_map.hpp>
#include <memoria/core/packed/map/packed_fse_mark_map.hpp>

#include <iostream>

namespace memoria    {


MEMORIA_ITERATOR_PART_BEGIN(memoria::metamap::ItrEntryName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Types::Entry                                    Entry;
    typedef typename Container::Value                                           Value;
    typedef typename Container::Key                                             Key;
    typedef typename Container::Accumulator                                     Accumulator;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Types::CtrSizeT                                 CtrSizeT;
    typedef typename Container::Types::Target                                   Target;
    typedef typename Container::Types::Position                                 Position;


    struct GetEntryFn: NodeWalkerBase<GetEntryFn, IntList<0>, IntList<0>> {
        template <Int Idx, typename Stream>
        Entry stream(const Stream* stream, Int idx)
        {
            MEMORIA_ASSERT_TRUE(stream);
            return metamap::GetEntry<Entry>(stream, idx);
        }
    };

    Entry getEntry() const
    {
        auto& self = this->self();
        return LeafDispatcher::dispatchConstRtn(self.leaf(), GetEntryFn(), self.idx());
    }


    struct SetEntryFn: NodeWalkerBase<SetEntryFn, IntList<0>, IntList<0>> {
        template <Int Idx, typename Stream>
        void stream(Stream* stream, Int idx, const Entry& entry)
        {
            MEMORIA_ASSERT_TRUE(stream);
            metamap::SetEntry(stream, idx, entry);
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

        entry.key()     = key;
        entry.value()   = value;

        self.ctr().insertEntry(self, entry);
    }

    void insert(const Entry& entry)
    {
        auto& self = this->self();
        self.ctr().insertEntry(self, entry);
    }

    Accumulator insert(const vector<Entry>& entries)
    {
        auto& self = this->self();

        MemBuffer<const Entry> buf(entries);

        return self.ctr().insert(self, buf);
    }

    CtrSizeT read(vector<Entry>& entries)
    {
        auto& self = this->self();

        MemBuffer<Entry> buf(entries);

        Target target(&buf);

        return self.ctr().readStream(self, target);
    }


    void remove(bool adjust_next_entry = true)
    {
        auto& self = this->self();

        Accumulator keys;
        self.ctr().removeMapEntry(self, keys);
    }

    void removeTo(MyType& to, bool adjust_next_entry = true)
    {
        auto& self = this->self();

        Accumulator keys;
        self.ctr().removeMapEntries(self, to, keys, adjust_next_entry);
    }

    void removeNext(CtrSizeT size, bool adjust_next_entry = true)
    {
        auto& self = this->self();

        auto to = self;

        to.skipFw(size);

        Accumulator keys;
        self.ctr().removeMapEntries(self, to, keys, adjust_next_entry);
    }


MEMORIA_ITERATOR_PART_END

}

#endif
