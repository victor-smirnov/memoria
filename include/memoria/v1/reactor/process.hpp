
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


#pragma once

#include <memoria/v1/core/types.hpp>
#include <memoria/v1/core/tools/pimpl_base.hpp>

#include <memoria/v1/reactor/pipe_streams.hpp>

namespace memoria {
namespace v1 {
namespace reactor {

class ProcessImpl;

class Process: public PimplBase<ProcessImpl> {
    using Base = PimplBase<ProcessImpl>;
public:
    MMA1_PIMPL_DECLARE_DEFAULT_FUNCTIONS(Process)

    int join();
    void terminate();

    PipeInputStream out_stream();
    PipeInputStream err_stream();
    PipeOutputStream in_stream();


    static Process create2(const U16String& path,
                          const std::vector<U16String>& args,
                          const std::vector<U16String>& env = std::vector<U16String>()
                         );

    static Process create(const U16String& path,
                          const U16String& args = u"",
                          const std::vector<U16String>& env = std::vector<U16String>()
                         );

};



}}}
