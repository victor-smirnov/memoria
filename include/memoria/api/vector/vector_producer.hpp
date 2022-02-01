
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

#include <memoria/core/datatypes/buffer/buffer.hpp>

#include <functional>

namespace memoria {

template <typename Types>
class VectorProducer: public io::IOVectorProducer {
public:
    using IOVSchema         = Linearize<typename Types::IOVSchema>;
    using ValuesSubstream   = DataTypeBuffer<typename Select<0, IOVSchema>::DataType>;
    using ProducerFn        = std::function<bool (ValuesSubstream&, size_t)>;

private:

    ProducerFn producer_fn_;

    size_t total_size_{};

public:

    VectorProducer(ProducerFn producer_fn):
        producer_fn_(producer_fn)
    {}

    size_t total_size() const {return total_size_;}

    virtual bool populate(io::IOVector& io_vector)
    {
        ValuesSubstream& buffer = io::substream_cast<ValuesSubstream>(io_vector.substream(0));

        bool has_more = producer_fn_(buffer, total_size_);

        total_size_ += buffer.size();

        io_vector.symbol_sequence().append_run(0, buffer.size());

        return has_more;
    }
};





}
