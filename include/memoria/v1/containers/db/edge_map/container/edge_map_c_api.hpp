
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


#include <memoria/v1/containers/db/edge_map/edge_map_names.hpp>
#include <memoria/v1/containers/db/edge_map/edge_map_tools.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(edge_map::CtrApiName)
public:
    using Types = typename Base::Types;
    using typename Base::IteratorPtr;

protected:
    using typename Base::NodeBaseG;
    using typename Base::NodeDispatcher;
    using typename Base::LeafDispatcher;
    using typename Base::BranchDispatcher;
    using typename Base::Position;
    using typename Base::BranchNodeEntry;
    using typename Base::BlockUpdateMgr;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using IOBuffer = DefaultIOBuffer;

public:
    IteratorPtr begin() {
        return self().template seek_stream<0>(0);
    }

    IteratorPtr end() {
        auto& self = this->self();
        return self.template seek_stream<0>(self.sizes()[0]);
    }

    IteratorPtr seek(CtrSizeT idx) {
        auto& self = this->self();
        return self.template seek_stream<0>(idx);
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    IteratorPtr find(Key key)
    {
        return self().template find_max_ge<IntList<0, 1>>(0, key);
    }

    IteratorPtr find_or_create(Key key)
    {
        auto& self = this->self();

        auto iter = self.find(key);

        if (!iter->is_found(key))
        {
            iter->insert_key(key);
        }

        return iter;
    }

    template <typename Iterator>
    IteratorPtr find_or_create(const Key& key, const Iterator& start, const Iterator& end)
    {
        auto& self = this->self();

        auto iter = self.find(key);

        if (!iter->is_found(key))
        {
            edge_map::EdgeMapEntryBufferProducer<IOBuffer, Key, Iterator> producer(key, start, end);

            iter->bulkio_insert(producer);
        }

        return iter;
    }

    bool upsert(const Key& key, const Value& value)
    {
        auto& self = this->self();

        auto iter = self.find_or_create(key);

        return iter->upsert_value(value);
    }


    bool remove(const Key& key, const Value& value)
    {
        auto& self = this->self();

        auto ii = self.find(key);

        if (ii->is_found(key))
        {
            auto size = ii->count_values();
            if (size > 0)
            {
                if (ii->remove_value(value))
                {
                    if (size == 1)
                    {
                        ii->to_prev_key();
                        ii->remove(1);
                    }

                    return true;
                }

                return false;
            }
            else {
                ii->remove(1);
                return false;
            }
        }

        return false;
    }

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(edge_map::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}
