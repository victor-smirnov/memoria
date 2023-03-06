
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

#include <memoria/asio/hrpc/session.hpp>
#include <memoria/asio/hrpc/context.hpp>
#include <memoria/asio/hrpc/call.hpp>

namespace memoria::asio::hrpc {

st::ContextImplPtr AsioHRPCSession::make_context(CallID call_id, const EndpointID& endpoint_id, Request rq)
{
    static thread_local auto pool
            = boost::make_local_shared<
                pool::SimpleObjectPool<AsioHRPCContext>
            >();

    auto ptr = pool->allocate_shared(this->shared_from_this(), call_id, endpoint_id, rq);
    ptr->post_create();
    return ptr;
}

st::CallImplPtr AsioHRPCSession::create_call(
        CallID call_id,
        Request request,
        st::CallCompletionFn completion_fn
) {
    static thread_local auto pool
            = boost::make_local_shared<
                pool::SimpleObjectPool<AsioHRPCCall>
            >();

    auto ptr = pool->allocate_shared(this->shared_from_this(), call_id, request, completion_fn);
    ptr->post_create();
    return ptr;
}


}
