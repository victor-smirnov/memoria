
// Copyright 2019 Victor Smirnov
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
#include <memoria/core/tools/assert.hpp>

#include <memoria/profiles/common/block_operations.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/core/packed/tools/packed_tools.hpp>


#include <memoria/core/tools/span.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/tools/result.hpp>

#include <memoria/core/packed/tools/packed_allocator_types.hpp>

#include <algorithm>

namespace memoria {

template <typename PkdStruct>
class PackedMapSO {

public:
    using MyType = PackedMapSO;

    using KeysSO   = typename PkdStruct::KeysPkdStruct::SparseObject;
    using ValuesSO = typename PkdStruct::ValuesPkdStruct::SparseObject;

    using KeysExtData   = typename PkdStruct::KeysPkdStruct::ExtData;
    using ValuesExtData = typename PkdStruct::ValuesPkdStruct::ExtData;

    using KeyView     = typename PkdStruct::KeyView;
    using ValueView   = typename PkdStruct::ValueView;

    using KeysUS  = typename KeysSO::UpdateState;
    using ValuesUS = typename ValuesSO::UpdateState;

    class UpdateState: public PkdStructUpdateBase<MyType> {
        KeysUS keys_update_state_;
        ValuesUS values_update_state_;
    public:
        UpdateState() {}

        KeysUS& keys_update_state()   {return keys_update_state_; }
        ValuesUS& values_update_state() {return values_update_state_;}

        void configure() {
            keys_update_state_.set_allocator_state(this->allocator_state());
            values_update_state_.set_allocator_state(this->allocator_state());
        }
    };

private:
    KeysExtData keys_ext_data_;
    ValuesExtData values_ext_data_;

    PkdStruct* data_;

    KeysSO keys_;
    ValuesSO values_;

public:

    using KeysPkdStruct         = typename PkdStruct::KeysPkdStruct;
    using ValuesPkdStruct       = typename PkdStruct::ValuesPkdStruct;

    PackedMapSO() noexcept:
        data_(), keys_(), values_()
    {}

    PackedMapSO(PkdStruct* data) noexcept:
        data_(data),
        keys_(&keys_ext_data_, data->keys()),
        values_(&values_ext_data_, data->values())
    {}

    PackedMapSO(const PkdStruct* data) noexcept:
        data_(const_cast<PkdStruct*>(data)),
        keys_(&keys_ext_data_, const_cast<PkdStruct*>(data)->keys()),
        values_(&values_ext_data_, const_cast<PkdStruct*>(data)->values())
    {}

    void setup() noexcept {
        keys_.setup();
        values_.setup();
        data_ = nullptr;
    }


    void setup(PkdStruct* data) noexcept
    {
        data_ = data;
        refresh_so();
    }

    operator bool() const noexcept {
        return data_ != nullptr;
    }

    const PkdStruct* data() const noexcept {return data_;}
    PkdStruct* data() noexcept {return data_;}

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("PACKED_MAP");

        keys_.generateDataEvents(handler);
        values_.generateDataEvents(handler);

        handler->endGroup();
        handler->endStruct();
    }

    void check() const
    {
        keys_.check();
        values_.check();

        auto keys_size = keys_.size();
        auto values_size = values_.size();

        if (keys_size != values_size)
        {
            MEMORIA_MAKE_GENERIC_ERROR(
                "PackedMap: keys size != values size: {} {}",
                keys_size,
                values_size
            ).do_throw();
        }
    }

    size_t size() const {
        return data_->size();
    }

    Optional<ValueView> find(const KeyView& key) const
    {
        size_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_LIKELY(key0 == key)) {
                return Optional<ValueView>(values_.access(0, result.local_pos()));
            }
        }

        return Optional<ValueView>();
    }


    PkdUpdateStatus prepare_set(const KeyView& key, const ValueView value, UpdateState& update_state) const
    {
        update_state.configure();

        size_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_UNLIKELY(key0 == key)) {
                return values_.prepare_update(result.local_pos(), 1, update_state.values_update_state(), [&](auto, auto){
                    return value;
                });
            }
            else {
                auto key_ss = keys_.prepare_insert(result.local_pos(), 1, update_state.keys_update_state(), [&](auto, auto){
                    return key;
                });

                if (!is_success(key_ss)) {
                    return PkdUpdateStatus::FAILURE;
                }

                auto values_ss = values_.prepare_insert(result.local_pos(), 1, update_state.values_update_state(), [&](auto, auto){
                    return value;
                });

                return values_ss;
            }
        }
        else {
            size_t idx = size;

            auto key_ss = keys_.prepare_insert(idx, 1, update_state.keys_update_state(), [&](auto, auto){
                return key;
            });

            if (!is_success(key_ss)) {
                return PkdUpdateStatus::FAILURE;
            }

            auto values_ss = values_.prepare_insert(idx, 1, update_state.values_update_state(), [&](auto, auto){
                return value;
            });

            return values_ss;
        }
    }

    void commit_set(const KeyView& key, const ValueView value, UpdateState& update_state)
    {
        update_state.configure();

        size_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_UNLIKELY(key0 == key)) {
                values_.commit_update(result.local_pos(), 1, update_state.values_update_state(), [&](auto, auto){
                    return value;
                });

                refresh_so();
            }
            else {
                keys_.commit_insert(result.local_pos(), 1, update_state.keys_update_state(), [&](auto, auto){
                    return key;
                });
                refresh_so();


                values_.commit_insert(result.local_pos(), 1, update_state.values_update_state(), [&](auto, auto){
                    return value;
                });

                refresh_so();
            }
        }
        else {
            size_t idx = size;

            keys_.commit_insert(idx, 1, update_state.keys_update_state(), [&](auto, auto){
                return key;
            });
            refresh_so();

            values_.commit_insert(idx, 1, update_state.values_update_state(), [&](auto, auto){
                return value;
            });

            refresh_so();
        }
    }

    void remove(const KeyView& key) noexcept
    {
        size_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_UNLIKELY(key0 == key))
            {
                keys_.remove(result.local_pos());
                refresh_so();

                values_.remove(result.local_pos());
                refresh_so();
            }
        }
    }



    void for_each(std::function<void (KeyView, ValueView)> fn) const
    {
        auto keys_ii  = keys_.begin(0);
        auto keys_end = keys_.end(0);

        auto values_ii  = values_.begin(0);
        auto values_end = values_.end(0);

        while (keys_ii != keys_end && values_ii != values_end)
        {
            fn(*keys_ii, *values_ii);
            ++keys_ii;
            ++values_ii;
        }
    }

    void for_each_noexcept(std::function<void (KeyView, ValueView)> fn) const noexcept
    {
        auto keys_ii  = keys_.begin(0);
        auto keys_end = keys_.end(0);

        auto values_ii  = values_.begin(0);
        auto values_end = values_.end(0);

        while (keys_ii != keys_end && values_ii != values_end)
        {
            fn(*keys_ii, *values_ii);

            ++keys_ii;
            ++values_ii;
        }
    }

    MMA_MAKE_UPDATE_STATE_METHOD

private:
    void refresh_so() noexcept {
        keys_.setup(data_->keys());
        values_.setup(data_->values());
    }
};


}
