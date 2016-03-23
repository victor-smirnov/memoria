
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/prototypes/bt_tl/bttl_names.hpp>
#include <memoria/v1/core/container/container.hpp>
#include <memoria/v1/core/container/macros.hpp>

#include <memoria/v1/prototypes/bt_tl/bttl_tools.hpp>


#include <vector>

namespace memoria {
namespace v1 {

MEMORIA_CONTAINER_PART_BEGIN(v1::bttl::InsertName)

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

        MEMORIA_ASSERT(locals[stream], >, 0);
        path[stream] += locals[stream] - 1;

        for (Int s = stream + 1; s < last_stream; s++)
        {
            MEMORIA_ASSERT(locals[s], >, 0);
            path[s] = locals[s] - 1;
        }

        path[last_stream] = locals[last_stream];

        iter = *self.seek(path, last_stream).get();

        return totals;
    }

MEMORIA_CONTAINER_PART_END

#define M_TYPE      MEMORIA_CONTAINER_TYPE(v1::bttl::InsertName)
#define M_PARAMS    MEMORIA_CONTAINER_TEMPLATE_PARAMS



#undef M_PARAMS
#undef M_TYPE

}}