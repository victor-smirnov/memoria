
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

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <memoria/v1/profiles/common/block_operations.hpp>

#include <memoria/v1/core/packed/tools/packed_allocator_types.hpp>
#include <memoria/v1/core/packed/tools/packed_tools.hpp>


#include <memoria/v1/core/tools/span.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/tools/optional.hpp>

#include <algorithm>

namespace memoria {
namespace v1 {

template <typename PkdStruct>
class PackedMapSO {

public:
    using KeysSO   = typename PkdStruct::KeysPkdStruct::SparseObject;
    using ValuesSO = typename PkdStruct::ValuesPkdStruct::SparseObject;

    using KeysExtData   = typename PkdStruct::KeysPkdStruct::ExtData;
    using ValuesExtData = typename PkdStruct::ValuesPkdStruct::ExtData;

    using KeyView   = typename PkdStruct::KeyView;
    using ValueView = typename PkdStruct::ValueView;

    using Key       = typename PkdStruct::Key;
    using Value     = typename PkdStruct::Value;

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

    void generateDataEvents(IBlockDataEventHandler* handler) const
    {
        handler->startStruct();
        handler->startGroup("PACKED_MAP");

        keys_.generateDataEvents(handler);
        values_.generateDataEvents(handler);

        handler->endGroup();
        handler->endStruct();
    }

    void check() const {
        data_->check();
    }

    psize_t size() const {
        return data_->size();
    }

    Optional<Value> find(const KeyView& key) const
    {
        int32_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA1_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA1_LIKELY(key0 == key)) {
                return Optional<Value>(values_.access(0, result.local_pos()));
            }
        }

        return Optional<Value>();
    }

    OpStatus set(const KeyView& key, const ValueView value)
    {
        int32_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA1_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA1_UNLIKELY(key0 == key))
            {
                if (isFail(values_.replace(0, result.local_pos(), value))) {
                    return OpStatus::FAIL;
                }
            }
            else {
                if (isFail(keys_.insert(result.local_pos(), key))) {
                    return OpStatus::FAIL;
                }

                refresh_so();

                if (isFail(values_.insert(result.local_pos(), value))) {
                    return OpStatus::FAIL;
                }

                refresh_so();
            }
        }
        else {
            psize_t idx = size;
            if (isFail(keys_.insert(idx, key))){
                return OpStatus::FAIL;
            }

            refresh_so();

            if (isFail(values_.insert(idx, value))){
                return OpStatus::FAIL;
            }

            refresh_so();
        }

        return OpStatus::OK;
    }

    OpStatus remove(const KeyView& key)
    {
        int32_t size = data_->size();
        auto result = keys_.findGEForward(0, key);
        if (MMA1_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA1_UNLIKELY(key0 == key))
            {
                if (isFail(keys_.remove(result.local_pos()))) {
                    return OpStatus::FAIL;
                }
                refresh_so();

                if (isFail(values_.remove(result.local_pos()))) {
                    return OpStatus::FAIL;
                }
                refresh_so();
            }
        }

        return OpStatus::OK;
    }

    psize_t estimate_required_upsize(const KeyView& key, const ValueView& value) const
    {
        psize_t upsize{};

        int32_t size = data_->size();

        auto result = keys_.findGEForward(0, key);
        if (MMA1_LIKELY(result.local_pos() < size))
        {
            KeyView key0 = keys_.access(0, result.local_pos());

            if (MMA1_LIKELY(key0 != key))
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

    psize_t estimate_required_upsize(const std::vector<std::pair<Key, Value>>& entries) const
    {
        psize_t upsize{};

        for (const auto& entry: entries) {
            upsize += estimate_required_upsize(std::get<0>(entry), std::get<1>(entry));
        }

        return upsize;
    }

    OpStatus set_all(const std::vector<std::pair<Key, Value>>& entries)
    {
        for (const auto& entry: entries)
        {
            const KeyView& key     = std::get<0>(entry);
            const ValueView& value = std::get<1>(entry);

            if (isFail(set(key, value))) {
                return OpStatus::FAIL;
            }
        }

        return OpStatus::OK;
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

private:
    void refresh_so() {
        keys_.setup(data_->keys());
        values_.setup(data_->values());
    }
};


}
}
