
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

#include "../fiber/all.hpp"

#include "reactor.hpp"

#ifndef _WIN32
#include "linux/smp.hpp"
#include "linux/socket_impl.hpp"
#else
#include "msvc/msvc_smp.hpp"
#include "msvc/msvc_socket_impl.hpp"
#endif



#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <functional>

namespace memoria {
namespace v1 {
namespace reactor {

class Application { //: public std::enable_shared_from_this<Application>
    
    std::shared_ptr<Smp> smp_;
    std::vector<std::shared_ptr<Reactor>> reactors_;
    
    std::function<void()> shutdown_hook_;
    
    static Application* application_;
    
public:
    Application(int argc, char** argv): Application(argc, argv, nullptr) {}
    Application( int argc, char** argv, char** envp );
    
    Application(const Application&) = delete;
    Application(Application&&) = delete;
    
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;
    
    
    void set_shutdown_hook(const std::function<void()>& fn) {
        shutdown_hook_ = fn;
    }
    
    
    template<typename Fn, typename... Args> 
    auto run(Fn&& fn, Args&&... args) 
    {
        auto msg = make_special_one_way_lambda_message(0, std::forward<Fn>(fn), std::forward<Args>(args)...);
        smp_->submit_to(0, msg.get());
        
        reactors_[0]->event_loop();
        
        for (auto& r: reactors_) 
        {
            r->join();
        }
        
        return msg->result();
    }
    
    friend Application& app();
    
    void shutdown() 
    {
        for (auto& r: reactors_) 
        {
            auto msg = new OneWayFunctionMessage(r->cpu(), shutdown_hook_);
            smp_->submit_to(r->cpu(), msg);
        }
    }
};

Application& app();
    
}}}
