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

#include <memoria/v1/core/types.hpp>

#ifdef MMA1_POSIX
#include "posix/posix_process_impl.hpp"
#elif defined (MMA1_WINDOWS)
#include "msvc/msvc_process_impl.hpp"
#endif


namespace memoria {
namespace v1 {
namespace reactor {


int Process::join() {
    return ptr_->join();
}

void Process::terminate() {
    return ptr_->terminate();
}

PipeInputStream Process::out_stream() {
    return PipeInputStream(ptr_->out_stream());
}

PipeInputStream Process::err_stream() {
    return PipeInputStream(ptr_->err_stream());
}

PipeOutputStream Process::in_stream() {
    return PipeOutputStream(ptr_->in_stream());
}



}}}
