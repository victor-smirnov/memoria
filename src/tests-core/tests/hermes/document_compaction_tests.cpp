
// Copyright 2019-2022 Victor Smirnov
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

#include "test_tools.hpp"

namespace memoria {
namespace tests {


auto ld_document_compaction_test = register_test_in_suite<FnTest<HermesTestState>>("HermesTestSuite", "DocumentCompaction", [](auto& state){
    auto doc = hermes::HermesCtr::make_new();

    auto map = doc->make_object_map();
    doc->set_root(map->as_object());

    size_t size = 100000;

    for (size_t c = 0; c < size; c++)
    {
        U8String key = "Entry" + std::to_string(c);
        map->put_dataobject<Varchar>(key, std::to_string(c));
    }

    auto m0 = doc->root().as_generic_map();

    assert_equals(size, m0->size());

    auto doc2 = doc->compactify();
    auto gmap = doc2->root().as_generic_map();

    assert_equals(size, gmap->size());

    for (size_t c = 0; c < size; c++)
    {
        U8String key = "Entry" + std::to_string(c);
        auto vv = gmap->get(hermes::HermesCtr::wrap_dataobject<Varchar>(key)->as_object());
        assert_equals(true, vv.is_not_empty());

        U8String value = std::to_string(c);
        assert_equals(value, vv.as_varchar());
    }

});

}}
