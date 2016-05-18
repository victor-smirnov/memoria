
// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/containers/multimap/mmap_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::mmap::ItrMiscName)

    using typename Base::NodeBaseG;
    using typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    static constexpr Int SizesSubstreamIdx = 2;

public:
    Key key() const
    {
        auto& self = this->self();

        MEMORIA_V1_ASSERT(self.stream(), ==, 0);

        return std::get<0>(self.template read_leaf_entry<0, IntList<1>>(self.idx(), 0));
    }

    Value value() const
    {
        auto& self = this->self();

        MEMORIA_V1_ASSERT(self.stream(), ==, 1);

        return std::get<0>(self.template read_leaf_entry<1, IntList<1>>(self.idx(), 0));
    }



    struct InsertKeyFn {

    	CtrSizeT one_;
    	CtrSizeT zero_;
    	const Key& key_;

    	InsertKeyFn(const Key& key): one_(1), zero_(0), key_(key) {}

    	const auto& get(const StreamTag<0>& , const StreamTag<0>&, Int block) const
    	{
    		return one_;
    	}

    	const auto& get(const StreamTag<0>& , const StreamTag<1>&, Int block) const
    	{
    		return key_;
    	}

    	const auto& get(const StreamTag<0>& , const StreamTag<SizesSubstreamIdx>&, Int block) const
    	{
    		return zero_;
    	}
    };

    void insert_key(const Key& key)
    {
    	auto& self = this->self();

    	MEMORIA_V1_ASSERT(self.stream(), ==, 0);

    	if (!self.isEnd())
    	{
    		self.ctr().template insert_stream_entry<0>(self, InsertKeyFn(key));
    	}
    	else {
    		if (self.prev())
    		{
    			auto pos = self.values_size();
    			self.toData(pos);

    			Int parent_idx = self.local_parent_idx(self.stream(), self.idx());

    			self.stream() = 0;
    			self.idx() 	  = parent_idx >= 0 ? parent_idx : 0;

    			self.ctr().template insert_stream_entry<0>(self, InsertKeyFn(key));
    		}
    		else {
    			self.idx() = 0;
    			self.ctr().template insert_stream_entry<0>(self, InsertKeyFn(key));
    		}
    	}

    	self.cache().data_size()[0] ++;
    	self.cache().data_size()[1] = 0;

    }

    std::vector<Value> read_values(CtrSizeT length = -1)
    {
        auto& self = this->self();

        std::vector<Value> values;

        self.scan_values(length, [&](auto&& value){
            values.push_back(value);
        });

        return values;
    }


    template <typename Fn>
    CtrSizeT scan_values(CtrSizeT length, Fn&& fn)
    {
        auto& self = this->self();

        if (self.stream() == 0) {
            self.toData();
        }

        auto data_size  = self.cache().data_size()[1];
        auto pos        = self.cache().data_pos()[1];

        if (length < 0 || pos + length > data_size)
        {
            length = data_size - pos;
        }

        SubstreamReadLambdaAdapter<Fn> adapter(fn);

        return self.ctr().template read_substream<IntList<1, 1>>(self, 0, length, adapter);
    }

    template <typename Fn>
    CtrSizeT scan_values(Fn&& fn)
    {
        auto& self = this->self();
        return self.scan_values(-1, std::forward<Fn>(fn));
    }

    template <typename Fn>
    CtrSizeT scan_keys(Fn&& fn)
    {
        auto& self = this->self();
        return self.scan_keys(-1, std::forward<Fn>(fn));
    }

    template <typename Fn>
    CtrSizeT scan_keys(CtrSizeT length, Fn&& fn)
    {
        auto& self = this->self();

        if (self.stream() == 0) {
            self.toIndex();
        }

        auto data_size  = self.cache().data_size()[0];
        auto pos        = self.cache().data_pos()[0];

        if (length < 0 || pos + length > data_size)
        {
            length = data_size - pos;
        }

        SubstreamReadLambdaAdapter<Fn> adapter(fn);

        return self.ctr().template read_substream<IntList<0, 0, 1>>(self, 0, length, adapter);
    }


    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().remove_subtrees(length);
    }

    CtrSizeT values_size() const {
        return self().substream_size();
    }


    template <typename Provider>
    auto bulk_insert(Provider&& provider, const Int initial_capacity = 2000)
    {
        auto& self = this->self();

        return self.ctr()._insert(self, std::forward<Provider>(provider), initial_capacity);
    }

    template <typename IOBuffer>
    auto bulkio_insert(BufferProducer<IOBuffer>& provider, const Int initial_capacity = 20000)
    {
    	auto& self = this->self();
    	return self.ctr().bulkio_insert(self, provider, initial_capacity);
    }


    bool is_found(const Key& key)
    {
    	auto& self = this->self();
    	if (self.stream() == 0)
    	{
    		return (!self.isEnd()) && self.key() == key;
    	}
    	else {
    		throw Exception(MA_SRC, SBuf() << "Current stream is not key stream");
    	}
    }


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::mmap::ItrMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
