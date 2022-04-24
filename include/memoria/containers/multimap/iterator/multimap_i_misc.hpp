
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

#include <memoria/core/types.hpp>

#include <memoria/containers/multimap/multimap_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/multimap/multimap_input.hpp>
#include <memoria/prototypes/bt/tools/bt_tools.hpp>

#include <memoria/core/tools/object_pool.hpp>

#include <memoria/containers/multimap/multimap_output_entries.hpp>
#include <memoria/containers/multimap/multimap_output_values.hpp>

#include <memoria/core/datatypes/traits.hpp>

#include <iostream>

namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(multimap::ItrMiscName)

    using typename Base::TreeNodePtr;
    using Container = typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    static constexpr int32_t DataStreams            = Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:

    Datum<Key> key() const
    {
        auto& self = this->self();

        int32_t stream = self.iter_data_stream();

        if (stream == 0)
        {
            int32_t key_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template iter_read_leaf_entry<0, IntList<1>>(0, key_idx));
        }
        else {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: {}", stream));
        }
    }

    Datum<Value> value() const
    {
        auto& self = this->self();

        int32_t stream = self.iter_data_stream();

        if (stream == 1)
        {
            int32_t value_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template iter_read_leaf_entry<1, IntList<1>>(0, value_idx));
        }
        else {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: ", stream));
        }
    }

    bool next_key()
    {
    	auto& self = this->self();

    	self.selectFw(1, 0);

    	return !self.iter_is_end();
    }


    CtrSizeT count_values() const
    {
        auto& self = this->self();
        int32_t stream = self.iter_data_stream();

        if (stream == 0)
        {
            auto ii = self.iter_clone();
            if (ii->next())
            {
                int32_t next_stream = ii->iter_data_stream_s();
                if (next_stream == 1)
                {
                    return ii->iter_count_fw();
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
            MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: {}", stream));
        }
    }

    CtrSizeT run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.iter_data_stream();
            if (stream == 1)
            {
                auto ii = self.iter_clone();
                return ii->iter_count_bw() - 1;
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

        if (!self.iter_is_end())
        {
            if (self.iter_data_stream() != 0)
            {
                MMA_THROW(Exception()) << WhatCInfo("Key insertion into the middle of data block is not allowed");
            }
        }

        self.template iter_insert_entry<0>(bt::SingleValueEntryFn<0, Key, CtrSizeT>(key));
    }

    void insert_value(const ValueView& value)
    {
        auto& self = this->self();
        
        // FIXME: Allows inserting into start of the sequence that is incorrect, 
        // but doesn't break the structure

        self.template iter_insert_entry<1>(bt::SingleValueEntryFn<1, Value, CtrSizeT>(value));
    }


    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().iter_remove_eq_nlt(length);
    }

    CtrSizeT values_size() const {
        return self().substream_size();
    }

    bool is_found(const KeyView& key)
    {
        auto& self = this->self();

        if (!self.iter_is_end())
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
    	int32_t stream = self.iter_data_stream();

        if (stream == 1) 
        {
            return true;
        }
        else {
            self.next();
            if (self.is_end())
            {
                return false;
            }
            else {
                return self.iter_data_stream() == 1;
            }
        }
    }

    void to_prev_key()
    {
        return self().selectBw(1, 0);
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(multimap::ItrMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
