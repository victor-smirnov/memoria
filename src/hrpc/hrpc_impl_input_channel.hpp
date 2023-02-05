
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

#include "hrpc_impl_common.hpp"

#include <list>

namespace memoria::hrpc {

class HRPCInputChannelImpl final:
        public InputChannel,
        public pool::enable_shared_from_this<HRPCInputChannelImpl>
{
    SessionImplPtr session_;
    CallID call_id_;
    ChannelCode channel_code_;
    bool closed_;
    bool call_side_;

    uint64_t batch_size_{};
    uint64_t batch_size_limit_;

    class UnboundedChannel {
        std::list<Message> messages_;
        fibers::mutex mutex_;
        fibers::condition_variable waiter_;
        bool closed_{};
    public:

        void push(Message&& msg) {
            messages_.push_front(std::move(msg));
            waiter_.notify_all();
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

        bool is_closed() const {return closed_;}

        void close() {
            closed_ = true;
            waiter_.notify_all();
        }

        void clean_and_close() {
            closed_ = true;
            messages_.clear();
            waiter_.notify_all();
        }

    private:
        bool should_wait() const {
            return messages_.size() == 0 && !closed_;
        }

        void wait_for_message() {
            std::unique_lock<fibers::mutex> lk(mutex_);
            waiter_.wait(lk, [&]() -> bool {
                return messages_.size() > 0 || closed_;
            });
        }
    };

    UnboundedChannel channel_;

public:
    HRPCInputChannelImpl(
        SessionImplPtr session,
        CallID call_id,
        ChannelCode stream_id,
        uint64_t batch_size_limit,
        bool call_side
    );

    ChannelCode code() override {
        return channel_code_;
    }

    PoolSharedPtr<Session> session() override;

    bool is_closed() override;

    bool pop(Message& msg) override;

    void close() override;

    void new_message(Message&& msg);
    void do_close_channel();
private:
};

}
