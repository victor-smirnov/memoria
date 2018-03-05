
// Copyright 2018 Victor Smirnov
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

#include <memoria/v1/yaml-cpp/yaml.h>

using namespace memoria::v1;
using namespace memoria::v1::reactor;

int main(int argc, char** argv)
{
    return Application::run(argc, argv, []{
        ShutdownOnScopeExit hh;
		
        YAML::Node node = YAML::Load("{name: Brewers, city: Milwaukee}");
        if (node["name"]) {
          std::cout << node["name"].as<std::string>() << "\n";
        }
        if (node["mascot"]) {
          std::cout << node["mascot"].as<std::string>() << "\n";
        }


        return 0;
    });
}
