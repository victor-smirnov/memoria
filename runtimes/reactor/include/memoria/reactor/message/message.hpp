
// Copyright 2017 Victor Smirnov
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

#include <memoria/core/memory/shared_ptr.hpp>
#include <memoria/core/tools/type_name.hpp>

#include <boost/fiber/context.hpp>

#include <boost/pool/object_pool.hpp>
#include <boost/smart_ptr/local_shared_ptr.hpp>

#include <exception>
#include <string>

namespace memoria {
namespace reactor {

using FiberContext = boost::fibers::context;


class Message: public MemoryObject {
protected:

    bool one_way_: 1;
    bool return_: 1;
    //bool ow_chainable_: 1;
    bool run_in_fiber_: 1;

    void* data_{};

    std::exception_ptr exception_;

public:
    Message(int cpu, bool one_way):
        one_way_(one_way),
        return_(false),
        run_in_fiber_(false)
    {
        this->owner_cpu_ = cpu;
    }
    
    virtual ~Message() noexcept {}
    
    int cpu() const {return owner_cpu_;}
    
    bool is_one_way() const {return one_way_;}
    bool is_return() const {return return_;}
    bool is_exception() const {return exception_ != nullptr;}
    //bool is_ow_chainable() const {return ow_chainable_;}
    bool is_run_in_fiber() const {return run_in_fiber_;}

    void* data() const {return data_;}
    
    void set_data(void* custom_data) {data_ = custom_data;}
    
    void rethrow() const {
        std::rethrow_exception(exception_);
    }
    
    virtual void process() noexcept = 0;
    virtual void finish() = 0;
    
    virtual std::string describe() = 0;
};


class DummyMessage: public Message {
public:
    DummyMessage(int cpu, bool one_way):
        Message(cpu, one_way)
    {}

    virtual void process() noexcept {}
    virtual void finish() {}

    virtual std::string describe() {
        return "DummyMessage";
    };

    static DummyMessage* make_instance(int cpu);
};


class MemoryMessage: public Message {
    MemoryObject* chain_;
public:
    MemoryMessage(int cpu, MemoryObject* chain):
        Message(cpu, true),
        chain_(chain)
    {}

    MemoryObject* chain() const {
        return chain_;
    }

    virtual void process() noexcept {
        chain_->run_finalizers();
    }

    virtual void finish() {}

    virtual std::string describe() {
        return std::string("MemoryMessage");
    };

    static MemoryMessage* make_instance(MemoryObject* obj);
};

template <typename T>
class MessagePool;


template <typename T>
class MessagePool: public boost::enable_shared_from_this<MessagePool<T>> {
    static_assert(std::is_base_of_v<Message, T>);
    size_t allocated_{};
    size_t max_{};
public:

    class MessageImpl final: public T {
        alignas (T) std::byte object_storage_[sizeof(T)];
        boost::local_shared_ptr<MessagePool> pool_;

    public:
        template <typename... Args>
        MessageImpl(boost::local_shared_ptr<MessagePool>&& pool, Args&&... args):
            T(std::forward<Args>(args)...),
            pool_(std::move(pool))
        {}

        virtual ~MessageImpl() noexcept  = default;

    protected:

        void finalize_memory_object() {
            pool_->release(this);
        }
    };

    friend class MessageImpl;

private:
    boost::object_pool<MessageImpl> alloc_;//{4096};

public:    
    ~MessagePool() {
        std::cout << "Delete MessagePool " << TypeNameFactory<T>::name() << std::endl;
    }

    template <typename... Args>
    T* allocate(Args&&... args) {
        allocated_++;

        if (allocated_ > max_) {
            max_ = allocated_;
            //std::cout << TypeNameFactory<T>::name() << " " << allocated_ << std::endl;
        }

        return new (alloc_.malloc()) MessageImpl(this->shared_from_this(), std::forward<Args>(args)...);
    }

private:
    void release(MessageImpl* holder) noexcept {
        alloc_.destroy(holder);
        allocated_--;
    }
};

}}
