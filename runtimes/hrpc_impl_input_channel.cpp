
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

#include <memoria/hrpc/hrpc_impl_input_channel.hpp>
#include <memoria/hrpc/hrpc_impl_session.hpp>

namespace memoria::hrpc::st {

HRPCInputChannelImpl::HRPCInputChannelImpl(
        SessionImplPtr session,
        CallID call_id,
        ChannelCode channel_code,
        uint64_t batch_size_limit,
        bool call_side
):
    session_(session), call_id_(call_id),
    channel_code_(channel_code), closed_(),
    call_side_(call_side),
    batch_size_limit_(batch_size_limit)
{}

PoolSharedPtr<Session> HRPCInputChannelImpl::session() {
    return session_;
}

}
