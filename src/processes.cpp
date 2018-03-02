
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


#include <memoria/v1/reactor/process.hpp>
#include <memoria/v1/reactor/application.hpp>

#include <memoria/v1/core/tools/type_name.hpp>

#include <iostream>

using namespace memoria::v1;
using namespace memoria::v1::reactor;


int main(int argc, char** argv, char** envp)
{
    return Application::run_e(argc, argv, envp, []{
        ShutdownOnScopeExit hh;

		engine().coutln(u"Image name: {}", app().image_name());

		for (auto& str : app().args()) 
		{
			engine().coutln(u"Arg: {}", str);
		}

		for (auto& str : app().env())
		{
			engine().coutln(u"Entry: {}", str);
		}



        Process process = Process::create(u"processes.exe", u"processes_abcd"); //c:\\msys64\\usr\\bin\\ls.exe
		//Process process = Process::create(u"c:\\msys64\\usr\\bin\\ls.exe", u""); 

		//Process process = Process::create(u"c:\\msys64\\usr\\bin\\sleep.exe", u"sleep 3");

        auto out = process.out_stream();

		
        while (!out.is_closed())
        {
            uint8_t buf[200];
            std::memset(buf, 0, sizeof(buf));

            size_t read = out.read(buf, sizeof(buf) - 1);

            if (read > 0) {
                engine().cout(u"{}", T2T<const char*>(buf)) << std::flush;
            }
        }
		

        process.kill();

        process.join();

        engine().coutln(u"Exit. Status: {}, exit code: {}", process.status(), process.exit_code());

        return 0;
    });
}
