
// Copyright 2016 Victor Smirnov
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

#include <memoria/v1/core/types.hpp>

#include <memoria/v1/containers/multimap/mmap_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/api/multimap/multimap_input.hpp>
#include <memoria/v1/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/v1/core/tools/object_pool.hpp>

#include <memoria/v1/containers/multimap/mmap_output_entries.hpp>
#include <memoria/v1/containers/multimap/mmap_output_values.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(multimap::ItrMiscName)

    using typename Base::NodeBaseG;
    using Container = typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;
    using KeyV      = typename DataTypeTraits<Key>::ValueType;
    using ValueV    = typename DataTypeTraits<Value>::ValueType;

    static constexpr int32_t DataStreams            = Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:

    KeyV key() const
    {
        auto& self = this->self();

        int32_t stream = self.data_stream();

        if (stream == 0)
        {
            int32_t key_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template iter_read_leaf_entry<0, IntList<1>>(key_idx, 0));
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid stream: {}", stream));
        }
    }

    ValueV value() const
    {
        auto& self = this->self();

        int32_t stream = self.data_stream();

        if (stream == 1)
        {
            int32_t value_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template iter_read_leaf_entry<1, IntList<1>>(value_idx, 0));
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid stream: ", stream));
        }
    }

    bool next_key()
    {
    	auto& self = this->self();

    	self.selectFw(1, 0);

    	return !self.isEnd();
    }


    CtrSizeT count_values() const
    {
        auto& self = this->self();
        int32_t stream = self.data_stream();

        if (stream == 0)
        {
            auto ii = self.iter_clone();
            if (ii->next())
            {
                int32_t next_stream = ii->data_stream_s();
                if (next_stream == 1)
                {
                    return ii->countFw();
                }
                else {
                    return 0;
                }
            }
            else {
                return 0;
            }
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid stream: {}", stream));
        }
    }

    CtrSizeT run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.data_stream();
            if (stream == 1)
            {
                auto ii = self.iter_clone();
                return ii->countBw() - 1;
            }
            else {
                return 0;
            }
        }
        else {
            return 0;
        }
    }

    CtrSizeT key_pos() const
    {
        auto& self = this->self();
        return self.rank(0);
    }

    void insert_key(const KeyView& key)
    {
        auto& self = this->self();

        if (!self.isEnd())
        {
            if (self.data_stream() != 0)
            {
                MMA1_THROW(Exception()) << WhatCInfo("Key insertion into the middle of data block is not allowed");
            }
        }

        self.template insert_entry<0>(bt::SingleValueEntryFn<0, Key, CtrSizeT>(key));
    }

    void insert_value(const ValueView& value)
    {
        auto& self = this->self();
        
        // FIXME: Allows inserting into start of the sequence that is incorrect, 
        // but doesn't break the structure

        self.template insert_entry<1>(bt::SingleValueEntryFn<1, Value, CtrSizeT>(value));
    }


    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().removeGE(length);
    }

    CtrSizeT values_size() const {
        return self().substream_size();
    }

    bool is_found(const KeyView& key)
    {
        auto& self = this->self();
        if (!self.isEnd())
        {
            return self.key() == key;
        }
        else {
            return false;
        }
    }

    bool to_values()
    {
    	auto& self = this->self();
    	int32_t stream = self.data_stream();

        if (stream == 1) 
        {
            return true;
        }
        else {
            self.next();
            if (self.is_end()) {
                return false;
            }
            else {
                return self.data_stream() == 1;
            }
        }
    }

    void to_prev_key()
    {
    	self().selectBw(1, 0);
    }




MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(multimap::ItrMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
