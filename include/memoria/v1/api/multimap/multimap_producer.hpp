
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

#include <memoria/v1/api/common/ctr_api_btfl.hpp>
#include <memoria/v1/api/common/iobuffer_adatpters.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <functional>

namespace memoria {
namespace v1 {

template <typename Types>
class MultimapProducer: public io::IOVectorProducer {
public:
    using IOVSchema         = Linearize<typename Types::IOVSchema>;

    using KeysSubstream     = DataTypeBuffer<typename Select<0, IOVSchema>::DataType>;
    using ValuesSubstream   = DataTypeBuffer<typename Select<1, IOVSchema>::DataType>;

    struct Sizes {
        size_t entries_{};
        size_t values_{};
    };


    using ProducerFn        = std::function<
        bool (
            io::IOSymbolSequence&,
            KeysSubstream&,
            ValuesSubstream&,
            Sizes&
        )
    >;

private:
    Sizes sizes_;
    ProducerFn producer_fn_;

public:

    MultimapProducer(ProducerFn producer_fn):
        producer_fn_(producer_fn)
    {}

    const Sizes& total_size() const {return sizes_;}

    virtual bool populate(io::IOVector& io_vector)
    {
        KeysSubstream& keys = io::substream_cast<KeysSubstream>(io_vector.substream(0));
        ValuesSubstream& values = io::substream_cast<ValuesSubstream>(io_vector.substream(1));

        return producer_fn_(io_vector.symbol_sequence(), keys, values, sizes_);
    }
};

}}
