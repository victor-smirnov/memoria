
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

#pragma once

#include <memoria/hrpc/hrpc_impl_session.hpp>

#include <memoria/seastar/hrpc/call.hpp>

#include <seastar/core/condition-variable.hh>
#include <seastar/core/thread.hh>

namespace memoria::hrpc::ss {

class SeastarHRPCSession:
    public st::HRPCSessionImpl,
    public pool::enable_shared_from_this<SeastarHRPCSession> {

    using Base = HRPCSessionImpl;

    seastar::condition_variable ngt_cvar_;

public:
    SeastarHRPCSession(
        st::EndpointRepositoryImplPtr endpoints,
        st::MessageProviderPtr message_provider,
        ProtocolConfig config,
        SessionSide session_side
    ):
        Base(endpoints, message_provider, config, session_side)
    {

    }

    void wait_for_negotiation() override {
        ngt_cvar_.wait([this](){
            return negotiated_;
        }).get();
    }

    void notify_negotiated() override {
        ngt_cvar_.broadcast();
    }

    void run_async(std::function<void()> fn) override {
        (void)seastar::async(std::move(fn));
    }

    st::SessionImplPtr self() override {
        return this->shared_from_this();
    }

    st::ContextImplPtr make_context(CallID call_id, const EndpointID& endpoint_id, Request rq) override;

    st::CallImplPtr create_call(
            CallID call_id,
            Request request,
            st::CallCompletionFn completion_fn
    ) override;
};

}

