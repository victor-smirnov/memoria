
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


#include <memoria/tests/state.hpp>

#include <memoria/core/exceptions/exceptions.hpp>
#include <memoria/core/strings/format.hpp>

#include <memoria/reactor/reactor.hpp>

namespace memoria {
namespace tests {

TestState::~TestState() noexcept {}

std::ostream& TestState::out() const {
    return reactor::engine().cout();
}

Optional<TestCoverage> coverage_from_string(const U8String& str)
{
    if (str == "smoke") {
        return TestCoverage::SMOKE;
    }
    else if (str == "tiny") {
        return TestCoverage::TINY;
    }
    else if (str == "small") {
        return TestCoverage::SMALL;
    }
    else if (str == "medium") {
        return TestCoverage::MEDIUM;
    }
    else if (str == "large") {
        return TestCoverage::LARGE;
    }
    else if (str == "xlarge") {
        return TestCoverage::XLARGE;
    }
    else {
        return Optional<TestCoverage>();
    }
}

}}
