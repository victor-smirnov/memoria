
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


#include "memoria/v1/fiber/all.hpp"
#include "memoria/v1/reactor/application.hpp"
#include "memoria/v1/filesystem/path.hpp"
#include "memoria/v1/filesystem/operations.hpp"
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/time.hpp>

#include <iostream>
#include <thread>
#include <vector>

namespace m  = memoria::v1;

namespace df  = memoria::v1::fibers;
namespace dr  = memoria::v1::reactor;
namespace mt  = memoria::v1::tools;
namespace fs  = memoria::v1::filesystem;

using namespace dr;

int main(int argc, char **argv) 
{    
    return Application::run(argc, argv, [](){
        ShutdownOnScopeExit hh;

        std::cout << "Hello from TCP Client!" << std::endl;

        ClientSocket cs(IPAddress(127,0,0,1), 5556);
        std::cout << "Connection established! " << std::endl;

        size_t data_size = 4096;
        uint8_t* data = new uint8_t[data_size];

        auto is = cs.input();
        auto os = cs.output();

        std::string str("Hello, world!");

        ssize_t written = os.write(mt::ptr_cast<uint8_t>(str.c_str()), str.length());

        std::cout << "written " << written << " bytes" << std::endl;

        ssize_t read = is.read(data, data_size);

        std::cout << "Read back: " << read << " bytes: " << std::string(mt::ptr_cast<char>(data), read) << std::endl;

        return 0;
    });
}
