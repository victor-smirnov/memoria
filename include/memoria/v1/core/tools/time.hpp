// Copyright 2015 Victor Smirnov
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

#include <memoria/v1/core/tools/strings/strings.hpp>
#include <memoria/v1/core/types/types.hpp>

#include <chrono>
#include <ostream>

namespace memoria {
namespace v1 {



int64_t  getTimeInMillis();
String FormatTime(int64_t millis);

static inline uint64_t getTimeInNanos() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

struct CallDuration {
    uint64_t total_time{};
    uint64_t total_calls{};
};

static inline std::ostream& operator<<(std::ostream& out, const CallDuration& stat) 
{
    if (stat.total_calls != 0) 
    {
        out << "[" << (stat.total_time / stat.total_calls) << ", " << stat.total_calls << "]";
    }
    else {
        out << "[0, 0]";
    }
    
    return out;
}


namespace detail {
    struct TimeHolder {
        CallDuration& stat;
        uint64_t start;
        TimeHolder(CallDuration& _stat): 
            stat(_stat), start(getTimeInNanos())
        {}
        
        ~TimeHolder() {
            stat.total_time += (getTimeInNanos() - start);
            stat.total_calls++;
        }
    };
}

template <typename Fn>
auto withTime(CallDuration& stat, Fn&& fn) {
    memoria::v1::detail::TimeHolder holder(stat);
    return fn();
}

}}
