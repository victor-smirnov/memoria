
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

#include <memoria/containers/db/update_log/update_log_names.hpp>
#include <memoria/core/container/iterator.hpp>
#include <memoria/core/container/macros.hpp>

#include <memoria/api/db/update_log/update_log_api.hpp>

#include <memoria/core/memory/ptr_cast.hpp>

#include <iostream>



namespace memoria {

MEMORIA_V1_ITERATOR_PART_BEGIN(update_log::ItrApiName)

    using typename Base::NodeBaseG;
    using Container = typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;

    using LeafDispatcher = typename Container::Types::Blocks::LeafDispatcher;

    static constexpr int32_t DataStreams            = Container::Types::DataStreams;
    static constexpr int32_t StructureStreamIdx     = Container::Types::StructureStreamIdx;

public:

    UUID snapshot_id() const
    {
        auto& self = this->self();

        int32_t stream = self.iter_data_stream();

        if (stream == 0)
        {
            int32_t key_idx = self.data_stream_idx(stream);

            return std::get<0>(self.template iter_read_leaf_entry<0, IntList<1>>(key_idx, 0));
        }
        else {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: {}", stream));
        }
    }

    UAcc128T ctr_name_i() const
    {
        auto& self = this->self();

        int32_t stream = self.iter_data_stream();

        if (stream == 1)
        {
            int32_t value_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template iter_read_leaf_entry<1, IntList<1>>(value_idx, 0));
        }
        else {
            MMA_THROW(Exception()) << WhatInfo(format_u8("Invalid stream: {}", stream));
        }
    }

    CtrSizeT snapshot_id_run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.iter_data_stream();
            if (stream == 0)
            {
                auto ii = self.iter_clone();

                auto r1 = ii->rank(0);
                ii->selectBw(1, 0);

                CtrSizeT r0{};

                if (ii->iter_local_pos() >= 0)
                {
                    r0 = ii->rank(0);
                }

                return r1 - r0;
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }


    CtrSizeT ctr_names_run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.iter_data_stream();
            if (stream == 1)
            {
                auto ii = self.iter_clone();

                auto r1 = ii->rank(1);
                ii->selectBw(1, 1);

                CtrSizeT r0{};

                if (ii->iter_local_pos() >= 0)
                {
                    r0 = ii->rank(1);
                }

                return r1 - r0;
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }

    CtrSizeT data_run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.iter_data_stream();
            if (stream == 2)
            {
                auto ii = self.iter_clone();
                return ii->iter_count_bw() - 1;
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }

    CtrSizeT data_run_size() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            int32_t stream = self.iter_data_stream();
            if (stream == 2)
            {
                auto ii = self.iter_clone();
                return ii->iter_count_fw() + self.data_run_pos();
            }
            else {
                return 0;
            }
        }
        else {
            return -1;
        }
    }


    /*
        bool next_key()
        {
            auto& self = this->self();

            self.selectFw(1, 0);

            return !self.iter_is_end();
        }

    void insert_key(const Key& key)
    {
        auto& self = this->self();

        if (!self.iter_is_end())
        {
            if (self.iter_data_stream() != 0)
            {
                MMA_THROW(Exception()) << WhatCInfo("Key insertion into the middle of data block is not allowed");
            }
        }

        self.template iter_insert_entry<0>(SingleValueEntryFn<0, Key, CtrSizeT>(key));
    }
*/


/*

    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().iter_remove_ge(length);
    }

    void to_prev_key()
    {
    	self().selectBw(1, 0);
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
    */

    CtrSizeT data_remove(CtrSizeT length) {
        return 0;
    }

    CtrSizeT data_seek(CtrSizeT pos)
    {
        auto& self = this->self();

        if (pos >= 0)
        {
            auto data_size = self.data_run_size();
            if (pos <= data_size)
            {
                auto current_pos = self.data_run_pos();

                if (pos > current_pos)
                {
                    self.iter_skip_fw(pos - current_pos);
                }
                else {
                    self.iter_skip_bw(current_pos - pos);
                }

                return pos;
            }
            else {
                MMA_THROW(RuntimeException()) << WhatCInfo("Position is begind the run's end");
            }
            return data_size;
        }
        else {
            MMA_THROW(RuntimeException()) << WhatCInfo("Position is negative");
        }
    }

    CtrSizeT seek_data_end()
    {
        auto& self = this->self();
        auto data_size = self.count_data_values();

        self.next();

        if (data_size > 0)
        {
            self.iter_skip_fw(data_size);
        }

        return data_size;
    }


    bool upsert_ctr_name(const UUID& ctr_name)
    {
        auto& self = this->self();

        UAcc128T ctr_name_i{ctr_name};

        auto result = self.find_ctr_name(ctr_name_i);

        if (result.is_found())
        {
            auto value_at = self.ctr_name_i() + result.prefix;

            if (value_at != ctr_name_i)
            {
                auto vv = ctr_name_i - result.prefix;
                self.insert_ctr_name(vv);
                self.iter_skip_fw(1);
                self.subtract_ctr_name(vv);

                return true;
            }

            return false;
        }
        else {
            self.insert_ctr_name(ctr_name_i - result.prefix);
            self.iter_skip_fw(1);
            return true;
        }
    }


    CtrSizeT count_ctr_names() const
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
                    ii->selectFw(1, 0);
                    auto r1 = ii->rank(1);
                    auto r0 = self.rank(1);

                    return r1 - r0;
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


    CtrSizeT count_data_values() const
    {
        auto& self = this->self();
        int32_t stream = self.iter_data_stream();

        if (stream == 1)
        {
            auto ii = self.iter_clone();
            if (ii->next())
            {
                int32_t next_stream = ii->iter_data_stream_s();
                if (next_stream == 2)
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




    UpdateLogFindResult find_ctr_name(const UAcc128T& ctr_name_i)
    {
        auto& self = this->self();

        auto ii = self.iter_clone();

        auto size = self.count_ctr_names();

        if (size > 0)
        {
            self.next();
            auto ii = self.iter_clone();
            auto values_start_pos = self.rank(1);

            self.iter_to_data_stream(1);

            typename Types::template FindGEForwardWalker<Types, IntList<1, 1>> walker(0, ctr_name_i);
            auto prefix = self.ctr_find_fw(walker);

            self.iter_stream() = StructureStreamIdx;

            auto values_end_pos = self.rank(1);

            auto actual_size = values_end_pos - values_start_pos;

            if (actual_size <= size)
            {
                auto pos = self.ctr_names_run_pos();
                return UpdateLogFindResult{prefix.template cast_to<UAcc128T::AccBitLength>(), pos, size};
            }
            else {
                self.iter_skip_bw(actual_size - size);

                auto values_start_prefix = ii->template sum_up<UAcc192T, IntList<1, 1>>(0);
                auto values_end_prefix = self.template sum_up<UAcc192T, IntList<1, 1>>(0);
                return UpdateLogFindResult{(values_end_prefix - values_start_prefix).template cast_to<UAcc128T::AccBitLength>(), size, size};
            }
        }
        else {
            self.next();
            return UpdateLogFindResult{UAcc128T{}, 0, 0};
        }
    }



    void insert_snapshot(const UUID& snapshot_id)
    {
        auto& self = this->self();
        self.template iter_insert_entry<0>(bt::SingleValueEntryFn<0, UUID, CtrSizeT>(snapshot_id));
    }


    void insert_ctr_name(const UAcc128T& value)
    {
        auto& self = this->self();

        // FIXME: Allows inserting into start of the sequence that is incorrect,
        // but doesn't break the structure

        self.template iter_insert_entry<1>(bt::SingleValueEntryFn<1, UAcc128T, CtrSizeT>(value));
    }

    void subtract_ctr_name(const UAcc128T& value)
    {
        auto& self = this->self();

        UAcc128T existing = self.ctr_name_i();
        self.template iter_update_entry<1, IntList<1>>(update_log::SingleValueUpdateEntryFn<1, UAcc128T, CtrSizeT>(existing - value));
    }

    void add_ctr_name(const UAcc128T& value)
    {
        auto& self = this->self();

        UAcc128T existing = self.ctr_name_i();
        self.template iter_update_entry<1, IntList<1>>(update_log::SingleValueUpdateEntryFn<1, UAcc128T, CtrSizeT>(existing + value));
    }

#ifdef MMA_USE_IOBUFFER
    auto make_snapshot_id_walker()
    {
        auto& self = this->self();
        return self.template create_scan_run_walker_handler<DefaultIOBuffer>(0);
    }

    auto make_ctr_name_walker()
    {
        auto& self = this->self();
        return self.template create_scan_run_walker_handler<DefaultIOBuffer>(1);
    }

    auto make_command_data_walker()
    {
        auto& self = this->self();
        return self.template create_scan_run_walker_handler<DefaultIOBuffer>(2);
    }

    auto make_io_buffer(size_t size = 65536)
    {
        auto& self = this->self();
        return self.ctr().pools().get_instance(PoolT<ObjectPool<DefaultIOBuffer>>()).get_unique(size);
    }

    template <typename WalkerPtr, typename BufferPtr>
    int32_t prefetch_snapshot_ids(WalkerPtr& walker, BufferPtr& iobuffer)
    {
        return walker->populate(iobuffer.get());
    }
#endif

MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(update_log::ItrApiName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}
