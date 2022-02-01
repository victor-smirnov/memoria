
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

#include <memoria/reactor/application.hpp>


#ifdef MMA_LINUX
#include "linux/linux_socket.hpp"
#elif defined(MMA_MACOSX)
#include "macosx/macosx_socket.hpp"
#elif defined(MMA_WINDOWS)
#include "msvc/msvc_socket.hpp"
#endif 


#ifdef MMA_POSIX
#include "posix/posix_application_impl.hpp"
#elif defined (MMA_WINDOWS)
#include "msvc/msvc_application_impl.hpp"
#endif


namespace memoria {
namespace reactor {

namespace detail {
	std::vector<U8String> arg_list_as_vector(const char* const * args)
	{
		std::vector<U8String> v;

		while (args && *args){
			v.emplace_back(*args);
			++args;
		}

		return v;
	}
}

ApplicationInit::ApplicationInit() {
    InitSockets();
}

ApplicationInit::~ApplicationInit() {
    DestroySockets();
}

Application* Application::application_;
    
Application::Application(const options_description& descr, int argc, char** argv, char** envp):
    descr_(descr),
    smp_{},
    reactors_(),
    shutdown_hook_([](){engine().stop();}),
    args_(detail::arg_list_as_vector(argv)),
    env_(Environment::create(envp))
{
    default_options(descr_);

    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, descr_),
        options_
    );
    boost::program_options::notify(options_);

    debug_ = options_["debug"].as<bool>();
    
    application_ = this;
    iopoll_timeout_ = options_["io-timeout"].as<uint64_t>();

    threads_ = options_["threads"].as<uint32_t>();
}

void Application::start_engines() {

    smp_ = std::make_shared<Smp>(threads_);

    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_.push_back(std::make_shared<Reactor>(smp_, c, c > 0));
    }

    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_[c]->start();
    }
}


int Application::start_main_event_loop()
{
    reactors_[0]->event_loop(iopoll_timeout_);
    return reactors_[0]->exit_status();
}


Application& app() {
    return *Application::application_;
}


Optional<U8String> Environment::get(const U8String& name) {
	return ptr_->get(name);
}

void Environment::set(const U8String& name, const U8String& value) {
	return ptr_->set(name, value);
}

EnvironmentMap Environment::entries() const {
	return ptr_->entries();
}

EnvironmentList Environment::entries_list() const {
	return ptr_->entries_list();
}

Environment Environment::create(const char* const* envp) {
    return Environment(std::make_shared<EnvironmentImpl>(envp));
}



    
}}
