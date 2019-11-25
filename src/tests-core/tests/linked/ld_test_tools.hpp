
// Copyright 2019 Victor Smirnov
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
#include <memoria/v1/tests/tests.hpp>
#include <memoria/v1/tests/assertions.hpp>

#include <memoria/v1/reactor/reactor.hpp>

#include <unordered_map>
#include <vector>

namespace memoria {
namespace v1 {
namespace tests {


struct LDTestState: TestState {};

template <typename T>
void assert_arrays_equal(const std::vector<T>& expected, const LDDArray& actual)
{
    assert_equals(expected.size(), actual.size());
    for (size_t c = 0; c < expected.size(); c++) {
        assert_equals(expected[c], actual.get(c).as_integer());
    }
}




template <typename K, typename V>
void assert_arrays_equal(const std::unordered_map<K, V>& expected, const LDDMap& actual)
{
    assert_equals(expected.size(), actual.size());
    for (auto ii: expected)
    {
        Optional<LDDValue> vv = actual.get(ii.first);
        assert_equals(ii.second, vv.get().as_integer());
    }

    actual.for_each([&](auto key, auto value){
        auto ii = expected.find(key);
        assert_equals(true, ii != expected.end());
        assert_equals(ii->second, value.as_integer());
    });
}

}
}}
