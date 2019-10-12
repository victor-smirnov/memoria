
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

#include <memoria/v1/core/iovector/io_vector.hpp>

namespace memoria {
namespace v1 {
namespace io {

/*
template <typename Types>
class MultimapEntryIOVector: public io::IOVector {

public:
    using Key   = typename Types::Key;
    using Value = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key>::ViewType;
    using ValueView = typename DataTypeTraits<Value>::ViewType;

    using KeyV   = typename DataTypeTraits<Key>::ValueType;
    using ValueV = typename DataTypeTraits<Value>::ValueType;

private:
    static constexpr int32_t SubstreamsNum = 2;

    PackedRLESymbolSequence<2> symbol_sequence_;

    using KeysSubstream = io::IO1DArraySubstreamView<Key>;
    using DataSubstream = io::IO1DArraySubstreamView<Value>;

    KeysSubstream keys_substream_;
    DataSubstream data_substream_;

    std::vector<int32_t> schema_{};

    core::StaticVector<IOSubstream*, 2> substreams_;

public:
    MultimapEntryIOVector(const KeyV* key, const ValueV* values, size_t values_size)
    {
        FixedSizeArrayColumnMetadata<KeyV> keys_meta{const_cast<KeyV*>(key), 1, 1};
        keys_substream_.configure(&keys_meta);
        data_substream_.configure(values, values_size);
        schema_.push_back(1);
        schema_.push_back(1);

        substreams_[0] = &keys_substream_;
        substreams_[1] = &data_substream_;

        symbol_sequence_.append(0, 1);
        if (values_size > 0) {
            symbol_sequence_.append(1, values_size);
        }
    }

    void reset()
    {
    }

    void add_substream(std::unique_ptr<IOSubstream>&& ptr)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    void add_stream_schema(int32_t substreams)
    {
        MMA1_THROW(UnsupportedOperationException());
    }

    IOSymbolSequence& symbol_sequence() {
        return symbol_sequence_;
    }

    const IOSymbolSequence& symbol_sequence() const {
        return symbol_sequence_;
    }

    size_t substreams() const {
        return SubstreamsNum;
    }

    IOSubstream& substream(size_t num) {
        return *substreams_[num];
    }

    const IOSubstream& substream(size_t num) const {
        return *substreams_[num];
    }

    int32_t streams() const {
        return schema_.size();
    }

    int32_t stream_size(int32_t stream) const
    {
        return schema_[stream];
    }

    void reindex(){}
};
*/

}}}
