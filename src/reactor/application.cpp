
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



namespace memoria {
namespace v1 {
namespace reactor {

Application* Application::application_;
    
Application::Application(int argc, char** argv, char** envp): 
    smp_(std::make_shared<Smp>(2)), 
    reactors_(),
    shutdown_hook_([](){engine().stop();})
{
    application_ = this;
    
    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_.push_back(std::make_shared<Reactor>(smp_, c, c > 0));
    }
    
    for (int c = 0; c < smp_->cpu_num(); c++)
    {
        reactors_[c]->start();
    }

#ifdef _MSC_VER
	WSADATA wsaData = { 0 };

	// Initialize Winsock
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		wprintf(L"WSAStartup failed: %d\n", iResult);
		std::terminate();
	}
#endif
}

Application& app() {
    return *Application::application_;
}
    
}}}
