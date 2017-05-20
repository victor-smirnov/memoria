
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
#include "linux/linux_smp.hpp"
#include "linux/linux_socket_impl.hpp"
#include "linux/linux_file.hpp"
#else
#include "msvc/msvc_smp.hpp"
#include "msvc/msvc_socket_impl.hpp"
#include "msvc/msvc_file.hpp"
#endif

#include <boost/program_options.hpp>

#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <functional>

namespace memoria {
namespace v1 {
namespace reactor {

class Application { //: public std::enable_shared_from_this<Application>
    
    using options_description   = boost::program_options::options_description;
    using variables_map         = boost::program_options::variables_map;
    
    variables_map options_;
    options_description descr_;
    
    std::shared_ptr<Smp> smp_;
    std::vector<std::shared_ptr<Reactor>> reactors_;
    
    std::function<void()> shutdown_hook_;
    
    static Application* application_;

public:

    //Application(int argc, char** argv): Application(argc, argv, nullptr) {}
    Application(int argc, char** argv, char** envp = nullptr): 
        Application(default_options(options_description()), argc, argv, envp) 
    {}
    
    Application(options_description descr, int argc, char** argv, char** envp = nullptr);
    
    
    Application(const Application&) = delete;
    Application(Application&&) = delete;
    
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;
    
    const variables_map& options() {return options_;}
    
    void set_shutdown_hook(const std::function<void()>& fn) {
        shutdown_hook_ = fn;
    }
    
    bool is_help() const {
        return options_.count("help") > 0;
    }
    
    
    template<typename Fn, typename... Args> 
    auto run(Fn&& fn, Args&&... args) 
    {
        auto msg = make_special_one_way_lambda_message(0, std::forward<Fn>(fn), std::forward<Args>(args)...);
        smp_->submit_to(0, msg.get());
        
        reactors_[0]->event_loop();
        
        join();
        
        return msg->result();
    }
    
    void print_options_and_shutdown() 
    {
        std::cout << descr_;
        run([this]{
            this->shutdown();
        });
    }
    
    
    void join() const {
        for (auto& r: reactors_) 
        {
            r->join();
        }
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
    
    
    static options_description default_options(options_description descr) 
    {
        descr.add_options() 
            ("help, h", "Prints command line switches")
            ("threads, t", boost::program_options::value<int32_t>()->default_value(1), "Specifies number of threads to use")
        ;
        
        return descr;
    }
    
    template <typename Fn, typename... Args>
    static int run(int argc, char** argv, Fn&& fn, Args&&... args) 
    {
        return run(
            boost::program_options::options_description(),
            argc, argv,
            std::forward<Fn>(fn), std::forward<Args>(args)...
        );
    }
    
    template <typename Fn, typename... Args>
    static int run(
        boost::program_options::options_description options,
        int argc, char** argv, Fn&& fn, Args&&... args
    ) 
    {
        Application app(argc, argv);
    
        if (app.is_help()) {
            app.print_options_and_shutdown();
            return 0;
        }
        else {
            return app.run(std::forward<Fn>(fn), std::forward<Args>(args)...);
        }
    }
};

Application& app();
    
}}}
