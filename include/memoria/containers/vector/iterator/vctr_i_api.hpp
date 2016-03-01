
// Copyright Victor Smirnov 2011+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CONTAINER_vctr_ITERATOR_API_HPP
#define _MEMORIA_CONTAINER_vctr_ITERATOR_API_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/dump.hpp>

#include <memoria/containers/vector/vctr_names.hpp>
#include <memoria/containers/vector/vctr_tools.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/prototypes/bt/bt_macros.hpp>

#include <iostream>

namespace memoria    {

MEMORIA_ITERATOR_PART_BEGIN(memoria::mvector::ItrApiName)

    typedef Ctr<typename Types::CtrTypes>                                       Container;

    typedef typename Base::Allocator                                            Allocator;
    typedef typename Base::NodeBase                                             NodeBase;
    typedef typename Base::NodeBaseG                                            NodeBaseG;

    typedef typename Container::Value                                           Value;
    typedef typename Container::BranchNodeEntry                                     BranchNodeEntry;

    typedef typename Container::Types::Pages::LeafDispatcher                    LeafDispatcher;
    typedef typename Container::Position                                        Position;

    using CtrSizeT = typename Container::Types::CtrSizeT;

    using InputBuffer = typename Container::Types::InputBuffer;

    template <typename InputIterator>
    auto bulk_insert(const InputIterator& start, const InputIterator& end)
    {
        mvector::VectorIteratorInputProvider<Container, InputIterator> provider(self().ctr(), start, end);
        return Base::bulk_insert(provider);
    }



    template <typename Iterator>
    class EntryAdaptor {
    	Iterator& current_;

    	Value value_;

    public:
    	EntryAdaptor(Iterator& current): current_(current) {}

    	template <typename V>
    	void put(StreamTag<0>, StreamTag<0>, Int block, V&& entry) {}

    	template <typename V>
    	void put(StreamTag<0>, StreamTag<1>, Int block, V&& value) {
    		value_ = value;
    	}

    	void next()
    	{
    		current_ = value_;
    		current_++;
    	}
    };


    template <typename OutputIterator>
    auto read(OutputIterator& iter, CtrSizeT length)
    {
    	auto& self = this->self();

    	EntryAdaptor<OutputIterator> adaptor(iter);

    	return self.ctr().template read_entries<0>(self, length, adaptor);
    }

    template <typename OutputIterator>
    auto read(OutputIterator& iter)
    {
    	auto& self = this->self();
    	return read(iter, self.ctr().size());
    }


    Value value() const
    {
        auto me = this->self();

        auto v = me.subVector(1);

        if (v.size() == 1)
        {
            return v[0];
        }
        else if (v.size() == 0)
        {
            throw Exception(MA_SRC, "Attempt to read vector after its end");
        }
        else {
            throw Exception(MA_SRC, "Invalid vector read");
        }
    }

    std::vector<Value> read(CtrSizeT size)
    {
    	auto& self = this->self();

    	auto pos = self.pos();
    	auto ctr_size = self.ctr().size();

    	auto length = pos + size <= ctr_size ? size : ctr_size - pos;

        std::vector<Value> data(length);

        auto iter = self;

        auto begin = data.begin();

        self.for_each(length, [&](const auto& entry) {
        	*begin = entry;
        	begin++;
        });

        return data;
    }

    struct ForEachFn {
    	template <typename StreamObj, typename Fn>
    	void stream(const StreamObj* obj, Int from, Int to, Fn&& fn)
    	{
    		obj->for_each(from, to, fn);
    	}
    };

    template <typename Fn>
    CtrSizeT for_each(CtrSizeT length, Fn&& fn)
    {
    	auto& self = this->self();

    	return self.ctr().template read_substream<IntList<0, 0, 1>>(self, 0, length, std::forward<Fn>(fn));
    }


    auto seek(CtrSizeT pos)
    {
    	auto& self = this->self();

    	CtrSizeT current_pos = self.pos();
        self.skip(pos - current_pos);
    }

MEMORIA_ITERATOR_PART_END

#define M_TYPE      MEMORIA_ITERATOR_TYPE(memoria::mvector::ItrApiName)
#define M_PARAMS    MEMORIA_ITERATOR_TEMPLATE_PARAMS



}

#undef M_TYPE
#undef M_PARAMS

#endif
