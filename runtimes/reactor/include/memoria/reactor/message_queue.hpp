
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

#include <memoria/reactor/message/message.hpp>
#include <memoria/reactor/mpmc_queue.hpp>

#include <memory>

namespace memoria::reactor {

class Reactor;

using MessageQueueT = MPMCQueue<Message*, 1024>;

class MessageQueue: std::shared_ptr<MessageQueueT> {
    using Base = std::shared_ptr<MessageQueueT>;

    friend class Reactor;

    MessageQueue(std::shared_ptr<MessageQueueT>&& queue):
        Base(std::move(queue))
    {}

public:
    MessageQueue():
        Base()
    {}

    MessageQueue(const MessageQueue& other):
        Base(other)
    {}

    MessageQueue(MessageQueue&& other):
        Base(std::move(other))
    {}

    ~MessageQueue() noexcept {}

    bool operator==(const MessageQueue& other) const noexcept {
        return get() == other.get();
    }

    MessageQueue& operator=(const MessageQueue& other) {
        Base::operator=(other);
        return *this;
    }

    MessageQueue& operator=(MessageQueue&& other) {
        Base::operator=(std::move(other));
        return *this;
    }

    static MessageQueue make() {
        return std::make_shared<MessageQueueT>();
    }

protected:
    template <typename Fn>
    void receive(Fn&& fn) {
        get()->get(64, std::forward<Fn>(fn));
    }
};

}
