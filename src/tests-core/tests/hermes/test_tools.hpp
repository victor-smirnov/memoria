
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


#include <memoria/core/types.hpp>
#include <memoria/tests/tests.hpp>
#include <memoria/tests/assertions.hpp>

#include <memoria/reactor/reactor.hpp>

#include <memoria/core/hermes/hermes.hpp>

#include <unordered_map>
#include <vector>

namespace memoria {
namespace tests {


struct HermesTestState: TestState {};

template <typename T>
void assert_arrays_equal(const std::vector<T>& expected, const LDDArrayView& actual)
{
    assert_equals(expected.size(), actual.size());
    for (size_t c = 0; c < expected.size(); c++) {
        assert_equals(expected[c], *actual.get(c)->as_bigint());
    }
}


template <typename T>
void assert_arrays_equal(const std::vector<T>& expected, hermes::GenericArrayPtr actual)
{
    assert_equals(expected.size(), actual->size());
    for (size_t c = 0; c < expected.size(); c++) {
        assert_equals(expected[c], actual->get(c)->cast_to(TypeTag<BigInt>{})->view());
    }
}



template <typename K, typename V>
void assert_arrays_equal(const std::unordered_map<K, V>& expected, const LDDMapView& actual)
{
    assert_equals(expected.size(), actual.size());
    for (auto ii: expected)
    {
        auto vv = actual.get(ii.first);
        assert_equals(ii.second, *vv->as_bigint());
    }

    actual.for_each([&](auto key, auto value){
        auto ii = expected.find(key);
        assert_equals(true, ii != expected.end());
        assert_equals(ii->second, *value.as_bigint());
    });
}

template <typename K, typename V>
void assert_arrays_equal(const std::unordered_map<K, V>& expected, hermes::GenericMapPtr actual)
{
    assert_equals(expected.size(), actual->size());
    for (auto ii: expected)
    {
        auto vv = actual->get(ii.first);
        assert_equals(ii.second, vv->as_bigint()->view());
    }

    actual->for_each([&](auto key, auto value){
        auto ii = expected.find(key);
        assert_equals(true, ii != expected.end());
        assert_equals(ii->second, value->cast_to(TypeTag<BigInt>{})->view());
    });
}

}}
