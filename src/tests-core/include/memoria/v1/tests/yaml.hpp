
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
#include <yaml-cpp/yaml.h>
#include <memoria/v1/filesystem/path.hpp>

#include <memoria/v1/api/store/memory_store_api.hpp>

#include <memoria/v1/tests/tests.hpp>

namespace memoria {
namespace v1 {
namespace tests {

template <typename T> struct IndirectStateFiledSerializer;


template <typename T>
struct IndirectStateFiledSerializer<AllocSharedPtr<IMemoryStore<T>>> {
    static void externalize(IMemoryStorePtr<T>& alloc, filesystem::path path, ConfigurationContext* context)
    {
        auto path_str = path.to_u8();
        path_str += ".mma1";
        alloc->store(path_str);
    }

    static void internalize(IMemoryStorePtr<T>& alloc, filesystem::path path, ConfigurationContext* context)
    {
        auto path_str = path.to_u8();
        path_str += ".mma1";
        alloc = IMemoryStore<T>::load(path_str);
    }
};

}
}}

namespace YAML {

template <typename T>
struct convert;

template <>
struct convert<memoria::v1::UUID> {
    static Node encode(const memoria::v1::UUID& rhs) {
        return Node(rhs.to_u8().data());
    }

    static bool decode(const Node& node, memoria::v1::UUID& rhs)
    {
        if (!node.IsScalar())
            return false;

        rhs = memoria::v1::UUID::parse(node.Scalar().c_str());

        return true;
    }
};

template <>
struct convert<memoria::v1::U8String> {
    static Node encode(const memoria::v1::U8String& rhs) {
        return Node(rhs.to_std_string());
    }

    static bool decode(const Node& node, memoria::v1::U8String& rhs)
    {
        if (!node.IsScalar())
            return false;

        rhs = memoria::v1::U8String(node.Scalar());

        return true;
    }
};

}


