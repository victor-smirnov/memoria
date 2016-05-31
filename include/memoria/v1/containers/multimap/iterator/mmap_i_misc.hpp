
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

#include <memoria/v1/core/types/types.hpp>

#include <memoria/v1/containers/multimap/mmap_names.hpp>
#include <memoria/v1/core/container/iterator.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <iostream>

namespace memoria {
namespace v1 {


MEMORIA_V1_ITERATOR_PART_BEGIN(v1::mmap::ItrMiscName)

    using typename Base::NodeBaseG;
    using typename Base::Container;
    using typename Base::BranchNodeEntry;
    using typename Base::CtrSizeT;
    using typename Base::CtrSizesT;


    using Key       = typename Container::Types::Key;
    using Value     = typename Container::Types::Value;

    using LeafDispatcher = typename Container::Types::Pages::LeafDispatcher;

    static constexpr Int DataStreams            = Container::Types::DataStreams;
    static constexpr Int StructureStreamIdx     = Container::Types::StructureStreamIdx;

    using IOBuffer  = DefaultIOBuffer;

public:
    Key key() const
    {
        auto& self = this->self();

        Int stream = self.data_stream();

        if (stream == 0)
        {
            Int key_idx = self.data_stream_idx(stream);

            return std::get<0>(self.template read_leaf_entry<0, IntList<1>>(key_idx, 0));
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
        }
    }

    Value value() const
    {
        auto& self = this->self();

        Int stream = self.data_stream();

        if (stream == 1)
        {
            Int value_idx = self.data_stream_idx(stream);
            return std::get<0>(self.template read_leaf_entry<1, IntList<1>>(value_idx, 0));
        }
        else {
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
        }
    }



    CtrSizeT count_values() const
    {
        auto& self = this->self();
        Int stream = self.data_stream();

        if (stream == 0)
        {
            auto ii = self.clone();
            if (ii->next())
            {
                Int next_stream = ii->stream_s();
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
            throw Exception(MA_SRC, SBuf() << "Invalid stream: " << stream);
        }
    }

    CtrSizeT run_pos() const
    {
        auto& self = this->self();
        if (!self.is_end())
        {
            Int stream = self.data_stream();
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
            return 0;
        }
    }


    void insert_key(const Key& key)
    {
        auto& self = this->self();

        if (!self.isEnd())
        {
            if (self.data_stream() != 0)
            {
                throw Exception(MA_SRC, "Key insertion into the middle of data block is not allowed");
            }
        }

        self.template insert_entry<0>(SingleValueEntryFn<0, Key, CtrSizeT>(key));
    }

    void insert_value(const Value& value)
    {
        auto& self = this->self();

        self.template insert_entry<1>(SingleValueEntryFn<1, Key, CtrSizeT>(value));
    }


    template <typename ValueConsumer>
    class ReadValuesFn: public BufferConsumer<IOBuffer> {
        IOBuffer io_buffer_;
        ValueConsumer* consumer_;

        CtrSizeT run_pos_;

        memoria::v1::rleseq::RLESymbolsRun run_;

    public:
        ReadValuesFn(Int capacity = 65536):
            io_buffer_(capacity),
            run_pos_(), run_()
        {}

        void init(ValueConsumer* consumer)
        {
            io_buffer_.rewind();

            run_pos_    = 0;
            run_        = memoria::v1::rleseq::RLESymbolsRun();
            consumer_ = consumer;
        }

        void clear() {}

        virtual IOBuffer& buffer() {return io_buffer_;}

        virtual Int process(IOBuffer& buffer, Int entries)
        {
            for (Int entry = 0; entry < entries; entry++)
            {
                if (run_pos_ == run_.length())
                {
                    run_ = buffer.template getSymbolsRun<DataStreams>();
                    run_pos_ = 0;
                    entry++;

                    if (entry == entries) {
                        return entries;
                    }
                }

                consumer_->emplace_back(IOBufferAdapter<Value>::get(buffer));
                run_pos_++;
            }

            return entries;
        }
    };


    std::vector<Value> read_values(CtrSizeT length = std::numeric_limits<CtrSizeT>::max())
    {
        auto& self = this->self();

        std::vector<Value> values;

        using ReadFn = ReadValuesFn<std::vector<Value>>;

        auto read_fn = self.ctr().pools().get_instance(PoolT<ObjectPool<ReadFn>>()).get_unique();
        read_fn->init(&values);

        self.bulkio_scan(read_fn.get(), 1, length);

        return values;
    }






    CtrSizesT remove(CtrSizeT length = 1)
    {
        return self().remove_subtrees(length);
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


MEMORIA_V1_ITERATOR_PART_END

#define M_TYPE      MEMORIA_V1_ITERATOR_TYPE(v1::mmap::ItrMiscName)
#define M_PARAMS    MEMORIA_V1_ITERATOR_TEMPLATE_PARAMS


#undef M_TYPE
#undef M_PARAMS

}}
