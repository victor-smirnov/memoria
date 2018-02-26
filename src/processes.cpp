
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


#include <memoria/v1/process/process.hpp>
#include <memoria/v1/reactor/application.hpp>

#include <iostream>

using namespace memoria::v1;
using namespace memoria::v1::reactor;

namespace bp = boost::process;

int main(int argc, char** argv)
{
    return Application::run(argc, argv, []{
        ShutdownOnScopeExit hh;


        bp::system("ls", bp::std_out > stdout, bp::std_err > stderr, bp::std_in < stdin);

        return 0;
    });
}
