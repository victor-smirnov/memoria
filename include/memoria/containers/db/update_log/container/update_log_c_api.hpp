
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


#include <memoria/containers/db/update_log/update_log_names.hpp>
#include <memoria/containers/db/update_log/update_log_tools.hpp>
#include <memoria/core/container/container.hpp>
#include <memoria/core/container/macros.hpp>

#include <vector>

namespace memoria {

MEMORIA_V1_CONTAINER_PART_BEGIN(update_log::CtrApiName)
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

    static const int32_t StructureStreamIdx = Types::StructureStreamIdx;

public:
    IteratorPtr begin() {
        return self().template ctr_seek_stream<0>(0);
    }

    IteratorPtr end() {
        auto& self = this->self();
        return self.template ctr_seek_stream<0>(self.sizes()[0]);
    }

    IteratorPtr seek(CtrSizeT idx) {
        auto& self = this->self();
        return self.template ctr_seek_stream<0>(idx);
    }

    CtrSizeT size() const {
        return self().sizes()[0];
    }

    /*
    IteratorPtr find(const UUID& snapshot_id)
    {
        return self().template ctr_find_max_ge<IntList<0, 1>>(0, snapshot_id);
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
            update_log::UpdateLogEntryBufferProducer<IOBuffer, Key, Iterator> producer(key, start, end);

            iter->insert_iovector(producer);
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
    */

    IteratorPtr latest_snapshot()
    {
        auto& self = this->self();
        auto ii = self.ctr_seq_end();

        if (ii->selectBw(1, 0) == 1)
        {
            return ii;
        }
        else {
            return IteratorPtr{nullptr};
        }
    }

    void create_snapshot(const UUID& snapshot_id)
    {
        auto& self = this->self();
        auto ii = self.ctr_seq_end();
        ii->insert_snapshot(snapshot_id);
    }

#ifdef MMA1_USE_IOBUFFER
    template <typename IOBuffer>
    void append_commands(const UUID& ctr_name, bt::BufferProducer<IOBuffer>& data_producer)
    {
        auto& self = this->self();
        auto ii = self.latest_snapshot();
        if (ii)
        {
            if (!ii->upsert_ctr_name(ctr_name))
            {
                ii->seek_data_end();
            }

            ii->insert_data_values(data_producer);
        }
        else {
            MMA1_THROW(RuntimeException()) << WhatCInfo("Can't find any snapshot to append to.");
        }
    }
#endif

    IteratorPtr read_commads(const UUID& ctr_name, CtrSizeT start = 0)
    {
        auto& self = this->self();
        auto ii = self.latest_snapshot();
        return IteratorPtr{nullptr};
    }

    bool remove_commands(const UUID& ctr_name, CtrSizeT start, CtrSizeT length)
    {
        return false;
    }

    bool remove_commands(const UUID& ctr_name)
    {
        return false;
    }

    IteratorPtr find_snapshot(const UUID& snapshot_id)
    {
        auto& self = this->self();
        auto ii = self.ctr_seq_end();

        while (ii->selectBw(1, 0) == 1)
        {
            if (ii->snapshot_id() == snapshot_id) {
                return ii;
            }
        }

        return IteratorPtr{nullptr};
    }

protected:

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(update_log::CtrApiName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}
