
// Copyright 2023 Victor Smirnov
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

#include <memoria/seastar/hrpc/context.hpp>

#include <memoria/seastar/hrpc/input_channel.hpp>
#include <memoria/seastar/hrpc/output_channel.hpp>


namespace memoria::hrpc::ss {



st::InputChannelImplPtr SeastarHRPCContext::make_input_channel(ChannelCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarHRPCInputChannel>>();

    return pool->allocate_shared(session_, call_id_, code, batch_size_limit_, false);
}

st::OutputChannelImplPtr SeastarHRPCContext::make_output_channel(ChannelCode code)
{
    static thread_local auto pool =
            boost::make_local_shared<pool::SimpleObjectPool<SeastarHRPCOutputChannel>>();

    return pool->allocate_shared(session_, call_id_, code, batch_size_limit_, false);
}

}
