
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

#include <memoria/hrpc/hrpc_impl_input_channel.hpp>

#include <seastar/core/condition-variable.hh>
#include <seastar/core/thread.hh>

namespace memoria::hrpc::ss {

class SeastarHRPCInputChannel final:
        public st::HRPCInputChannelImpl,
        public pool::enable_shared_from_this<SeastarHRPCInputChannel>
{
    using Base = st::HRPCInputChannelImpl;

protected:

    class UnboundedChannel {
        std::list<Message> messages_;
        seastar::condition_variable waiter_;
        bool closed_{};
    public:

        void push(Message&& msg) {
            messages_.push_front(std::move(msg));
            waiter_.broadcast();
        }

        bool pop(Message& target)
        {
            if (should_wait()) {
                wait_for_message();
            }

            if (messages_.size()) {
                target = std::move(messages_.back());
                messages_.pop_back();
                return true;
            }

            return false;
        }

        bool is_closed() const {return closed_ && messages_.empty();}

        void close() {
            closed_ = true;
            waiter_.broadcast();
        }

        void clean_and_close() {
            closed_ = true;
            messages_.clear();
            waiter_.broadcast();
        }

    private:
        bool should_wait() const {
            return messages_.size() == 0 && !closed_;
        }

        void wait_for_message()
        {
            waiter_.wait([&]() -> bool {
                return messages_.size() > 0 || closed_;
            }).get();
        }
    };

    UnboundedChannel channel_;

public:
    SeastarHRPCInputChannel(
        st::SessionImplPtr session,
        CallID call_id,
        ChannelCode stream_id,
        uint64_t batch_size_limit,
        bool call_side
    ): Base(session, call_id, stream_id, batch_size_limit, call_side)
    {}

    ChannelCode code() override {
        return channel_code_;
    }

    PoolSharedPtr<st::Session> session() override;

    bool is_closed() override;

    bool pop(Message& msg) override;

    void close() override;

    void new_message(Message&& msg) override;
    void do_close_channel() override;
private:
};


}
