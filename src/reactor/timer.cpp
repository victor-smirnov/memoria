
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


#ifdef __linux__
#include "linux/linux_timer.hpp"
#elif __APPLE__
#include "macosx/macosx_timer.hpp"
#elif _WIN32
#include "msvc/msvc_timer.hpp"
#else
#error "Unsupported platform"
#endif


#include <memoria/reactor/reactor.hpp>


namespace memoria {
namespace reactor {

Timer Timer::schedule(TimeUnit start_after, TimeUnit repeat_after, uint64_t count, TimerFn fn) {
    return MakeLocalShared<TimerImpl>(start_after, repeat_after, count, fn);
}


bool Timer::is_running() const {
    return this->ptr_->is_running();
}

void Timer::cancel() {
    return this->ptr_->cancel();
}


}}
