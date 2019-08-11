
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

#include <memoria/v1/api/common/ctr_output_btfl_entries.hpp>
#include <memoria/v1/api/common/ctr_output_btfl_run.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/static_array.hpp>
#include <memoria/v1/core/iovector/io_symbol_sequence.hpp>

#include <memory>
#include <tuple>
#include <functional>
#include <vector>

namespace memoria {
namespace v1 {

namespace _ {

    template <
        typename DataType,
        typename ViewType = typename DataTypeTraits<DataType>::ViewType,
        bool IsFixedSize = DataTypeTraits<DataType>::isFixedSize
    >
    class MMapSubstreamAdapter;

    template <typename DataType, typename ViewType>
    class MMapSubstreamAdapter<DataType, ViewType, true> {
        Span<const ViewType> array_;

    public:
        auto& array() {return array_;}
        const auto& array() const {return array_;}
        Span<const ViewType> span() const {return array_;}
        void clear() {}
    };


    template <typename DataType, typename ViewType>
    class MMapSubstreamAdapter<DataType, ViewType, false> {
        io::DefaultIOBuffer<ViewType> array_;

    public:
        auto& array() {return array_;}
        const auto& array() const {return array_;}
        Span<const ViewType> span() const {return array_;}
        void clear() {
            array_.clear();
        }
    };




    template <
        typename DataType,
        typename ViewType = typename DataTypeTraits<DataType>::ViewType,
        bool IsFixedSize = DataTypeTraits<DataType>::isFixedSize
    >
    class MMapValueGroupsAdapter;


    template <typename DataType, typename ViewType>
    class MMapValueGroupsAdapter<DataType, ViewType, true> {

        io::DefaultIOBuffer<Span<const ViewType>> array_;

    public:
        auto& array() {return array_;}
        const auto& array() const {return array_;}
        Span<const Span<const ViewType>> span() const {return array_.span();}
        void clear() {
            array_.clear();
        }

        template <typename SubstreamAdapter, typename Parser>
        auto populate(const io::IOSubstream& substream, Parser& parser, int32_t values_offset, size_t start, size_t end)
        {
            const ViewType* values = SubstreamAdapter::select(substream, 0, values_offset);

            size_t keys_num{};
            size_t values_end = values_offset;

            for (size_t c = start; c < end;)
            {
                auto& run = parser.buffer()[c];
                if (MMA1_LIKELY(run.symbol == 0))
                {
                    keys_num++;
                    uint64_t keys_run_length = run.length;
                    if (MMA1_LIKELY(keys_run_length == 1)) // typical case
                    {
                        auto& next_run = parser.buffer()[c + 1];

                        if (MMA1_LIKELY(next_run.symbol == 1))
                        {
                            uint64_t values_run_length = next_run.length;
                            array_.append_value(absl::Span<const ViewType>(values, values_run_length));

                            values += values_run_length;
                            values_end += values_run_length;
                            c += 2;
                        }
                        else {
                            // zero-length value
                            array_.append_value(absl::Span<const ViewType>(values, 0));
                            c += 1;
                        }
                    }
                    else
                    {
                        // A series of keys with zero-length values
                        for (uint64_t s = 0; s < keys_run_length; s++)
                        {
                            array_.append_value(absl::Span<const ViewType>(nullptr, 0));
                        }

                        c += 1;
                    }
                }
                else {
                    MMA1_THROW(RuntimeException()) << WhatCInfo("Unexpected NON-KEY symbol in streams structure");
                }
            }

            return std::make_tuple(values_end, keys_num);
        }
    };


    template <typename DataType, typename ViewType>
    class MMapValueGroupsAdapter<DataType, ViewType, false>
    {
        io::DefaultIOBuffer<Span<const ViewType>> array_;
        io::DefaultIOBuffer<ViewType> values_;

    public:
        auto& array() {return array_;}
        const auto& array() const {return array_;}
        auto& values() {return values_;}
        const auto& values() const {return values_;}

        Span<const Span<const ViewType>> span() const {return array_.tail_ptr();}
        void clear() {
            values_.clear();
            array_.clear();
        }
    };

}

template <typename Types, typename Profile>
class IEntriesIterator {

public:
    using Key_   = typename Types::Key;
    using Value_ = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key_>::ViewType;
    using ValueView = typename DataTypeTraits<Value_>::ViewType;

    using Key   = typename DataTypeTraits<Key_>::ValueType;
    using Value = typename DataTypeTraits<Value_>::ValueType;

    using IOVSchema = Linearize<typename Types::IOVSchema>;

protected:

    using KeysIOVSubstreamAdapter   = IOSubstreamAdapter<Select<0, IOVSchema>>;
    using ValuesIOVSubstreamAdapter = IOSubstreamAdapter<Select<1, IOVSchema>>;

    core::StaticVector<uint64_t, 2> offsets_;
    DefaultBTFLSequenceParser<2> parser_;

protected:

    _::MMapSubstreamAdapter<Key_> keys_;
    _::MMapValueGroupsAdapter<Value_> values_;

    _::MMapSubstreamAdapter<Value_> prefix_;


    bool has_suffix_;

    KeyView suffix_key_;
    _::MMapSubstreamAdapter<Value_> suffix_;

    uint64_t iteration_num_{};
public:
    IEntriesIterator(uint32_t start):
        parser_(start)
    {}

    virtual ~IEntriesIterator() noexcept {}

    Span<const ValueView> prefix() const {return prefix_.span();}

    const KeyView& suffix_key_view() const {return suffix_key_;}
    Key suffix_key() const {return suffix_key_;}

    Span<const ValueView> suffix() const {return suffix_.span();}

    bool has_prefix() const {return prefix_.array().size() > 0;}
    bool has_suffix() const {return has_suffix_;}

    bool is_first_iteration() const {return iteration_num_ == 1;}

    Span<const KeyView> keys() const
    {
        return keys_.span();
    }

    Span<const Span<const ValueView>> values() const
    {
        return values_.span();
    }


    bool has_entries() const {
        return values_.array().size() > 0;
    }


    virtual bool is_end() const     = 0;
    virtual void next()             = 0;
    virtual void dump_iterator() const = 0;

};


template <typename Types, typename Profile>
class IValuesIterator {
public:
    using Value_ = typename Types::Value;
    using ValueView = typename DataTypeTraits<Value_>::ViewType;
    using Value = typename DataTypeTraits<Value_>::ValueType;
protected:
    const Value* values_{};
    size_t size_{};

    bool use_buffered_{};

    bool buffer_is_ready_{false};

public:
    IValuesIterator()
    {}

    virtual ~IValuesIterator() noexcept {}

    Span<const Value> buffer() const {
        return Span<const Value>{values_, size_};
    }

    bool is_buffer_ready() const {
        return buffer_is_ready_;
    }

    void read_buffer()
    {
        set_buffered();
        while (!is_buffer_ready()) {
            next();
        }
    }

    bool is_buffered() const {return use_buffered_;}
    virtual void set_buffered() = 0;

    virtual bool is_end() const     = 0;
    virtual void next()             = 0;
    virtual void dump_iterator() const = 0;
};


template <typename Types, typename Profile>
class IKeysIterator {
public:
    using Key_ = typename Types::Key;
    using Value_ = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key_>::ViewType;
    using ValueView = typename DataTypeTraits<Value_>::ViewType;

    using Key   = typename DataTypeTraits<Key_>::ValueType;
    using Value = typename DataTypeTraits<Value_>::ValueType;
protected:
    const Key* keys_{};
    size_t size_{};

    bool use_buffered_{};
    bool buffer_is_ready_{false};

public:
    IKeysIterator() {}

    virtual ~IKeysIterator() noexcept {}

    Span<const Key> buffer() const {
        return Span<const Key>{keys_, size_};
    }

    virtual CtrSharedPtr<IValuesIterator<Types, Profile>> values(size_t key_idx) = 0;

    virtual bool is_end() const         = 0;
    virtual void next()                 = 0;
    virtual void dump_iterator() const  = 0;
};

}}
