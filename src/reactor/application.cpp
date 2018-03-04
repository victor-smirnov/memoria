
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


#ifdef MMA1_LINUX
#include "linux/linux_socket.hpp"
#elif defined(MMA1_MACOSX)
#include "macosx/macosx_socket.hpp"
#elif defined(MMA1_WINDOWS)
#include "msvc/msvc_socket.hpp"
#endif 


#ifdef MMA1_POSIX
#include "posix/posix_application_impl.hpp"
#elif defined (MMA1_WINDOWS)
#include "msvc/msvc_application_impl.hpp"
#endif


namespace memoria {
namespace v1 {
namespace reactor {

namespace _ {
	std::vector<U16String> arg_list_as_vector(const char* const * args)
	{
		std::vector<U16String> v;

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
    
Application::Application(options_description descr, int argc, char** argv, char** envp): 
    descr_(descr),
    smp_{},
    reactors_(),
    shutdown_hook_([](){engine().stop();}),
	args_(_::arg_list_as_vector(argv)),
	env_(Environment::create(envp)),
	image_name_(_::get_image_name(args_)),
	image_name_u8_(image_name_.to_u8())
{
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, descr), 
        options_
    );
    boost::program_options::notify(options_);

    debug_ = options_["debug"].as<bool>();
    
    application_ = this;

	smp_ = std::make_shared<Smp>(options_["threads"].as<uint32_t>());

    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_.push_back(std::make_shared<Reactor>(smp_, c, c > 0));
    }
    
    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_[c]->start();
    }
}

Application& app() {
    return *Application::application_;
}


Optional<U16String> Environment::get(const U16String& name) {
	return ptr_->get(name);
}

void Environment::set(const U16String& name, const U16String& value) {
	return ptr_->set(name, value);
}

EnvironmentMap Environment::entries() const {
	return ptr_->entries();
}

EnvironmentList Environment::entries_list() const {
	return ptr_->entries_list();
}

Environment Environment::create(const char* const* envp) {
	return Environment(make_shared<EnvironmentImpl>(envp));
}

    
}}}
