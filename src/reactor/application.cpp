
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

#include <memoria/v1/reactor/application.hpp>


#ifdef __linux__
#include "linux/linux_socket.hpp"
#elif __APPLE__
#include "macosx/macosx_socket.hpp"
#elif _WIN32
#include "msvc/msvc_socket.hpp"
#else 
#error "Unsupported platform"
#endif 



namespace memoria {
namespace v1 {
namespace reactor {

ApplicationInit::ApplicationInit() {
    InitSockets();
}

ApplicationInit::~ApplicationInit() {
    DestroySockets();
}

Application* Application::application_;
    
Application::Application(options_description descr, int argc, char** argv, char** envp): 
    descr_(descr),
    smp_{},
    reactors_(),
    shutdown_hook_([](){engine().stop();})
{
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, descr), 
        options_
    );
    boost::program_options::notify(options_);
    
	smp_ = std::make_shared<Smp>(options_["threads"].as<uint32_t>());
	
    debug_ = options_["debug"].as<bool>();
    
    application_ = this;
   
    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_.push_back(std::make_shared<Reactor>(smp_, c, c > 0));
    }
    
    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_[c]->start();
    }

    InitSockets();
}

Application& app() {
    return *Application::application_;
}
    
}}}
