
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

#include <algorithm>

namespace memoria {

template <typename PkdStruct>
class PackedMapSO {

public:
    using KeysSO   = typename PkdStruct::KeysPkdStruct::SparseObject;
    using ValuesSO = typename PkdStruct::ValuesPkdStruct::SparseObject;

    using KeysExtData   = typename PkdStruct::KeysPkdStruct::ExtData;
    using ValuesExtData = typename PkdStruct::ValuesPkdStruct::ExtData;

    using KeyView   = typename PkdStruct::KeyView;
    using ValueView = typename PkdStruct::ValueView;

private:
    KeysExtData keys_ext_data_;
    ValuesExtData values_ext_data_;

    PkdStruct* data_;

    KeysSO keys_;
    ValuesSO values_;

    using MyType = PackedMapSO;

public:

    using KeysPkdStruct         = typename PkdStruct::KeysPkdStruct;
    using ValuesPkdStruct       = typename PkdStruct::ValuesPkdStruct;

    PackedMapSO():
        data_(), keys_(), values_()
    {}

    PackedMapSO(PkdStruct* data):
        data_(data),
        keys_(&keys_ext_data_, data->keys()),
        values_(&values_ext_data_, data->values())
    {}

    void setup() {
        keys_.setup();
        values_.setup();
        data_ = nullptr;
    }


    void setup(PkdStruct* data)
    {
        data_ = data;
        refresh_so();
    }

    operator bool() const {
        return data_ != nullptr;
    }

    const PkdStruct* data() const {return data_;}
    PkdStruct* data() {return data_;}

    VoidResult generateDataEvents(IBlockDataEventHandler* handler) const noexcept
    {
        handler->startStruct();
        handler->startGroup("PACKED_MAP");

        MEMORIA_TRY_VOID(keys_.generateDataEvents(handler));
        MEMORIA_TRY_VOID(values_.generateDataEvents(handler));

        handler->endGroup();
        handler->endStruct();

        return VoidResult::of();
    }

    void check() const {
        data_->check();
    }

    psize_t size() const {
        return data_->size();
    }

    Optional<ValueView> find(const KeyView& key) const
    {
        int32_t size = data_->size();
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

    VoidResult set(const KeyView& key, const ValueView value) noexcept
    {
        int32_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_UNLIKELY(key0 == key))
            {
                MEMORIA_TRY_VOID(values_.replace(0, result.local_pos(), value));
            }
            else {
                MEMORIA_TRY_VOID(keys_.insert(result.local_pos(), key));

                refresh_so();

                MEMORIA_TRY_VOID(values_.insert(result.local_pos(), value));

                refresh_so();
            }
        }
        else {
            psize_t idx = size;
            MEMORIA_TRY_VOID(keys_.insert(idx, key));

            refresh_so();

            MEMORIA_TRY_VOID(values_.insert(idx, value));

            refresh_so();
        }

        return VoidResult::of();
    }

    VoidResult remove(const KeyView& key) noexcept
    {
        int32_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_UNLIKELY(key0 == key))
            {
                MEMORIA_TRY_VOID(keys_.remove(result.local_pos()));
                refresh_so();

                MEMORIA_TRY_VOID(values_.remove(result.local_pos()));
                refresh_so();
            }
        }

        return VoidResult::of();
    }

    psize_t estimate_required_upsize(const KeyView& key, const ValueView& value) const
    {
        psize_t upsize{};

        int32_t size = data_->size();

        auto result = keys_.findGEForward(0, key);
        if (MMA_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA_LIKELY(key0 != key))
            {
                upsize += keys_.estimate_insert_upsize(key);
                upsize += values_.estimate_insert_upsize(value);
            }
            else {
                upsize += values_.estimate_replace_upsize(result.local_pos(), value);
            }
        }
        else {
            upsize += keys_.estimate_insert_upsize(key);
            upsize += values_.estimate_insert_upsize(value);
        }

        return upsize;
    }

    psize_t estimate_required_upsize(const std::vector<std::pair<KeyView, ValueView>>& entries) const
    {
        psize_t upsize{};

        for (const auto& entry: entries) {
            upsize += estimate_required_upsize(std::get<0>(entry), std::get<1>(entry));
        }

        return upsize;
    }

    VoidResult set_all(const std::vector<std::pair<KeyView, ValueView>>& entries) noexcept
    {
        for (const auto& entry: entries)
        {
            const KeyView& key     = std::get<0>(entry);
            const ValueView& value = std::get<1>(entry);

            MEMORIA_TRY_VOID(set(key, value));
        }

        return VoidResult::of();
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

    VoidResult for_each_noexcept(std::function<VoidResult (KeyView, ValueView)> fn) const noexcept
    {
        auto keys_ii  = keys_.begin(0);
        auto keys_end = keys_.end(0);

        auto values_ii  = values_.begin(0);
        auto values_end = values_.end(0);

        while (keys_ii != keys_end && values_ii != values_end)
        {
            MEMORIA_TRY_VOID(fn(*keys_ii, *values_ii));

            ++keys_ii;
            ++values_ii;
        }

        return VoidResult::of();
    }

private:
    void refresh_so() {
        keys_.setup(data_->keys());
        values_.setup(data_->values());
    }
};


}
