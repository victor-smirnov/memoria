
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

#include <memoria/v1/containers/db/edge_map/edge_map_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/api/db/edge_map/edge_map_api.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(edge_map::ItrMiscName)

    using typename Base::NodeBaseG;
    using Container = typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Blocks::LeafDispatcher;

    static constexpr int32_t DataStreams            = Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:

    Key key() const
    {
        auto& self = this->self();

        int32_t stream = self.data_stream();

        if (stream == 0)
        {
            int32_t key_idx = self.data_stream_idx(stream);

            return std::get<0>(self.template read_leaf_entry<0, IntList<1>>(key_idx, 0));
        }
        else {
            MMA1_THROW(Exception()) << WhatInfo(fmt::format8(u"Invalid stream: {}", stream));
        }
    }

    Value value() const
    {
        auto& self = this->self();

        int32_t stream = self.data_stream();

        if (stream == 1)
        {
            int32_t value_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template read_leaf_entry<1, IntList<1>>(value_idx, 0));
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



    CtrSizeT run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.data_stream();
            if (stream == 1)
            {
                auto ii = self.clone();
                return ii->countBw() - 1;
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }


    void insert_key(const Key& key)
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

    void insert_value(const Value& value)
    {
        auto& self = this->self();
        
        // FIXME: Allows inserting into start of the sequence that is incorrect, 
        // but doesn't break the structure

        self.template insert_entry<1>(bt::SingleValueEntryFn<1, Value, CtrSizeT>(value));
    }

    void subtract_value(const Value& value)
    {
        auto& self = this->self();

        Value existing = self.value();
        self.template update_entry<1, IntList<1>>(edge_map::SingleValueUpdateEntryFn<1, Value, CtrSizeT>(existing - value));
    }

    void add_value(const Value& value)
    {
        auto& self = this->self();

        Value existing = self.value();
        self.template update_entry<1, IntList<1>>(edge_map::SingleValueUpdateEntryFn<1, Value, CtrSizeT>(existing + value));
    }

#ifdef MMA1_USE_IOBUFFER

    template <typename ValueConsumer>
    class ReadValuesFn: public bt::BufferConsumer<IOBuffer> {

        ValueConsumer* consumer_;

    public:

        ReadValuesFn(ValueConsumer* consumer)
        {
            consumer_ = consumer;
        }

        void clear() {}

        virtual int32_t process(IOBuffer& buffer, int32_t entries)
        {
            for (int32_t entry = 0; entry < entries; entry++) {
                consumer_->emplace_back(IOBufferAdapter<Value>::get(buffer));
            }

            return entries;
        }
    };

#endif




    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().removeGE(length);
    }

    CtrSizeT values_size() const {
        return self().substream_size();
    }



    bool is_found(const Key& key)
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




    CtrSizeT count_values() const
    {
        auto& self = this->self();
        int32_t stream = self.data_stream();

        if (stream == 0)
        {
            auto ii = self.clone();
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


    CtrSizeT count_values_skip()
    {
        auto& self = this->self();
        int32_t stream = self.data_stream();

        if (stream == 0)
        {
            if (self.next())
            {
                int32_t next_stream = self.data_stream_s();
                if (next_stream == 1)
                {
                    return self.countFw();
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



    EdgeMapFindResult find_value(const Value& value)
    {
        auto& self = this->self();

        auto ii = self.clone();

        auto size = self.count_values();

        if (size > 0)
        {
            self.to_values();
            auto ii = self.clone();
            auto values_start_pos = self.pos();

            self.toDataStream(1);

            typename Types::template FindGEForwardWalker<Types, IntList<1, 1>> walker(0, value);
            auto prefix = self.find_fw(walker);

            self.stream() = StructureStreamIdx;

            auto values_end_pos = self.pos();

            auto actual_size = values_end_pos - values_start_pos;

            if (actual_size <= size)
            {
                auto pos = self.run_pos();
                return EdgeMapFindResult{prefix.template cast_to<Value::AccBitLength>(), pos, size};
            }
            else {
                self.skipBw(actual_size - size);

                auto values_start_prefix = ii->template sum_up<UAcc192T, IntList<1, 1>>(0);
                auto values_end_prefix = self.template sum_up<UAcc192T, IntList<1, 1>>(0);
                return EdgeMapFindResult{(values_end_prefix - values_start_prefix).template cast_to<Value::AccBitLength>(), size, size};
            }
        }
        else {
            self.next();
            return EdgeMapFindResult{Value{}, 0, 0};
        }
    }

    bool upsert_value(const Value& value)
    {
        auto& self = this->self();

        auto result = self.find_value(value);

        if (result.is_found())
        {
            auto value_at = this->value() + result.prefix;

            if (value_at != value)
            {
                auto vv = value - result.prefix;
                self.insert_value(vv);
                self.skipFw(1);

                self.subtract_value(vv);
                return true;
            }

            return false;
        }
        else {
            self.insert_value(value - result.prefix);
            return true;
        }
    }

    bool contains(const Value& value)
    {
        auto& self = this->self();

        auto result = self.find_value(value);

        return result.is_found();
    }

    bool remove_value(const Value& value)
    {
        auto& self = this->self();

        auto result = self.find_value(value);

        if (result.is_found())
        {
            auto vv = this->value();

            auto value_at = vv + result.prefix;

            if (value_at == value)
            {
                self.remove(1);

                if (result.pos + 1 < result.size)
                {
                    self.add_value(vv);
                }

                return true;
            }
        }

        return false;
    }

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(edge_map::ItrMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
