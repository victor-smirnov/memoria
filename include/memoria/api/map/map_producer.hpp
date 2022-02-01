
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

#include <memoria/api/common/ctr_api_btss.hpp>

#include <memoria/core/datatypes/traits.hpp>
#include <memoria/core/datatypes/encoding_traits.hpp>
#include <memoria/core/datatypes/io_vector_traits.hpp>

#include <memoria/core/iovector/io_vector.hpp>

#include <memoria/api/common/iobuffer_adatpters.hpp>

#include <functional>

namespace memoria {


template <typename Types>
class MapProducer: public io::IOVectorProducer {
public:
    using IOVSchema         = Linearize<typename Types::IOVSchema>;
    using KeysSubstream     = DataTypeBuffer<typename Select<0, IOVSchema>::DataType>;
    using ValuesSubstream   = DataTypeBuffer<typename Select<1, IOVSchema>::DataType>;
    using ProducerFn        = std::function<bool (KeysSubstream&, ValuesSubstream&, size_t)>;

private:
    ProducerFn producer_fn_;

    size_t total_size_{};

public:

    MapProducer(ProducerFn producer_fn):
        producer_fn_(producer_fn)
    {}

    size_t total_size() const {return total_size_;}

    virtual bool populate(io::IOVector& io_vector)
    {
        KeysSubstream& keys = io::substream_cast<KeysSubstream>(io_vector.substream(0));
        ValuesSubstream& values = io::substream_cast<ValuesSubstream>(io_vector.substream(1));

        bool has_more = producer_fn_(keys, values, total_size_);

        total_size_ += keys.size();

        io_vector.symbol_sequence().append_run(0, keys.size());

        return has_more;
    }
};

}
