
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

#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_V1_CONTAINER_PART_BEGIN(v1::bttl::InsertName)

public:
    using Types             = typename Base::Types;
    using Iterator          = typename Base::Iterator;

protected:


    static const Int Streams = Types::Streams;

    template <typename Provider>
    auto _insert(Iterator& iter, Provider&& provider, const Int total_capacity = 2000)
    {
        auto& self = this->self();

        auto path = iter.cache().data_pos();

        auto stream = iter.stream();

        bttl::StreamingCtrInputProvider<MyType, Provider> streamingProvider(self, provider, stream, total_capacity);

        auto pos = iter.local_stream_posrank_();

        streamingProvider.prepare(iter, pos);

        auto result = self.insert_provided_data(iter.leaf(), pos, streamingProvider);

        auto totals = streamingProvider.totals();
        auto locals = streamingProvider.locals();

        Int last_stream = streamingProvider.last_symbol();

        MEMORIA_V1_ASSERT(locals[stream], >, 0);
        path[stream] += locals[stream] - 1;

        for (Int s = stream + 1; s < last_stream; s++)
        {
            MEMORIA_V1_ASSERT(locals[s], >, 0);
            path[s] = locals[s] - 1;
        }

        path[last_stream] = locals[last_stream];

        iter = *self.seek(path, last_stream).get();

        return totals;
    }

MEMORIA_V1_CONTAINER_PART_END

#define M_TYPE      MEMORIA_V1_CONTAINER_TYPE(v1::bttl::InsertName)
#define M_PARAMS    MEMORIA_V1_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}