
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

#include <memoria/api/common/ctr_output_btfl_entries.hpp>
#include <memoria/api/common/ctr_output_btfl_run.hpp>

#include <memoria/api/multimap/multimap_output_adapters.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/tools/static_array.hpp>
#include <memoria/core/iovector/io_symbol_sequence.hpp>

#include <memory>
#include <tuple>
#include <functional>
#include <vector>

namespace memoria {

template <typename Types, typename Profile>
class IEntriesScanner {

public:
    using Key_   = typename Types::Key;
    using Value_ = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key_>::ViewType;
    using ValueView = typename DataTypeTraits<Value_>::ViewType;

    using IOVSchema = Linearize<typename Types::IOVSchema>;

protected:

    using KeysIOVSubstreamAdapter   = IOSubstreamAdapter<Select<0, IOVSchema>>;
    using ValuesIOVSubstreamAdapter = IOSubstreamAdapter<Select<1, IOVSchema>>;

    using ValuesBufferAtomType      = typename ValuesIOVSubstreamAdapter::AtomType;

    core::StaticVector<uint64_t, 2> offsets_;
    DefaultBTFLSequenceParser<2> parser_;

protected:

    _::MMapSubstreamAdapter<Key_> keys_;
    _::MMapValueGroupsAdapter<Value_> values_;

    _::MMapSubstreamAdapter<Value_> prefix_;


    bool has_suffix_;

    KeyView suffix_key_;
    _::MMapSubstreamAdapter<Value_> suffix_;

    _::MMapValuesBufferAdapter<Value_, ValuesBufferAtomType> suffix_buffer_;

    uint64_t iteration_num_{};

public:

    using EntryProcessorFn = std::function<void (uint64_t, KeyView, Span<const ValueView>, bool, bool)>;
    using BufferedEntryProcessorFn = std::function<void (KeyView, Span<const ValueView>)>;

public:
    IEntriesScanner(uint32_t start):
        parser_(start)
    {}

    virtual ~IEntriesScanner() noexcept {}

    void for_each(EntryProcessorFn proc)
    {
        if (!is_end())
        {
            KeyView no_key{};

            bool has_prefix = false;
            uint64_t seq_id{};

            while (!is_end())
            {
                if (has_prefix)
                {
                    bool is_run_end = has_suffix();
                    proc(seq_id, no_key, prefix(), false, is_run_end);

                    if (is_run_end) {
                        seq_id++;
                    }
                }

                auto keys   = this->keys();
                auto values = this->values();

                for (size_t c = 0; c < keys.size(); c++, seq_id++)
                {
                    proc(seq_id, keys[c], values[c], true, true); // fully available values
                }

                if (has_suffix())
                {
                    proc(seq_id, suffix_key_view(), suffix(), true, false);
                    has_prefix = true;
                }

                next();
            }

            proc(seq_id, no_key, Span<const ValueView>(), false, true);
        }
    }


    void for_each(BufferedEntryProcessorFn proc)
    {
        if (!is_end())
        {
            Datum<Key_> last_suffix_key{};

            bool has_prefix = false;

            while (!is_end())
            {
                if (has_prefix)
                {
                    proc(last_suffix_key, suffix_buffer_.span());
                }

                auto keys   = this->keys();
                auto values = this->values();

                for (size_t c = 0; c < keys.size(); c++)
                {
                    proc(keys[c], values[c]); // fully available values
                }

                if (has_suffix()) {
                    last_suffix_key = suffix_key();
                }

                fill_suffix_buffer().throw_if_error();

                has_prefix = true;
            }

            proc(last_suffix_key, suffix_buffer_.span());
        }
    }

    Span<const ValueView> prefix() const {return prefix_.span();}

    const KeyView& suffix_key_view() const {return suffix_key_;}
    Datum<Key_> suffix_key() const {return suffix_key_;}

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


    virtual VoidResult fill_suffix_buffer() noexcept = 0;

    virtual bool is_end() const = 0;
    virtual BoolResult next() noexcept = 0;


    virtual void dump_iterator() const = 0;

};


template <typename Types, typename Profile>
class IValuesScanner {
public:
    using Value_ = typename Types::Value;
    using ValueView = typename DataTypeTraits<Value_>::ViewType;

    using IOVSchema = Linearize<typename Types::IOVSchema>;


protected:
    using ValuesIOVSubstreamAdapter = IOSubstreamAdapter<Select<1, IOVSchema>>;
    using ValuesBufferAtomType      = typename ValuesIOVSubstreamAdapter::AtomType;

    _::MMapSubstreamAdapter<Value_> values_;
    _::MMapValuesBufferAdapter<Value_, ValuesBufferAtomType> values_buffer_;

    size_t size_{};

    bool run_is_finished_{};

public:
    IValuesScanner()
    {}

    virtual ~IValuesScanner() noexcept {}

    Span<const ValueView> values() const {
        return values_.span();
    }

    Span<const ValueView> buffer() const {
        return values_buffer_.span();
    }

    bool is_run_finished() const {return run_is_finished_;}

    virtual VoidResult fill_suffix_buffer() noexcept = 0;


    virtual bool is_end() const     = 0;
    virtual BoolResult next_block() noexcept = 0;
    virtual void dump_iterator() const = 0;
};


template <typename Types, typename Profile>
class IKeysScanner {
public:
    using Key_ = typename Types::Key;
    using Value_ = typename Types::Value;

    using KeyView   = typename DataTypeTraits<Key_>::ViewType;
    using ValueView = typename DataTypeTraits<Value_>::ViewType;

    using IOVSchema = Linearize<typename Types::IOVSchema>;

protected:

    using KeysIOVSubstreamAdapter = IOSubstreamAdapter<Select<0, IOVSchema>>;

    _::MMapSubstreamAdapter<Key_> keys_;

public:
    IKeysScanner() {}

    virtual ~IKeysScanner() noexcept {}

    Span<const KeyView> keys() const {
        return keys_.span();
    }

    virtual CtrSharedPtr<IValuesScanner<Types, Profile>> values(size_t key_idx) = 0;

    virtual bool is_end() const         = 0;
    virtual VoidResult next() noexcept  = 0;
    virtual void dump_iterator() const  = 0;
};

}
