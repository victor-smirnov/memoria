
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

#include <memoria/v1/api/common/ctr_api_btss.hpp>

#include <memoria/v1/api/datatypes/traits.hpp>
#include <memoria/v1/api/datatypes/encoding_traits.hpp>
#include <memoria/v1/api/datatypes/io_vector_traits.hpp>

#include <memoria/v1/core/iovector/io_vector.hpp>

#include <memoria/v1/api/common/iobuffer_adatpters.hpp>

#include <functional>

namespace memoria {
namespace v1 {

template <typename Types>
class MapProducer: public io::IOVectorProducer {
public:
    using IOVSchema         = Linearize<typename Types::IOVSchema>;
    using KeysSubstream     = IOSubstreamAdapter<Select<0, IOVSchema>>;
    using ValuesSubstream   = IOSubstreamAdapter<Select<1, IOVSchema>>;
    using ProducerFn       = std::function<bool (KeysSubstream&, ValuesSubstream&, size_t)>;

private:
    KeysSubstream keys_{0};
    ValuesSubstream values_{0};

    ProducerFn producer_fn_;

    size_t total_size_{};

public:

    MapProducer(ProducerFn producer_fn):
        producer_fn_(producer_fn)
    {}

    size_t total_size() const {return total_size_;}

    virtual bool populate(io::IOVector& io_vector)
    {
        keys_.reset(io_vector.substream(0));
        values_.reset(io_vector.substream(1));

        bool has_more = producer_fn_(keys_, values_, total_size_);

        total_size_ += keys_.size_;

        io_vector.symbol_sequence().append(0, keys_.size_);

        return has_more;
    }
};

}}
