
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

#include <memoria/v1/core/types.hpp>

#include <vector>
#include <ostream>

#include <boost/optional.hpp>

namespace memoria {
namespace v1 {

template <typename T>
class OptionalT {
    T value_;
    bool is_set_;
public:
    using ValueType = T;

    template <typename TT>
    OptionalT(TT&& value): value_(value), is_set_(true) {}

    template <typename TT>
    OptionalT(TT&& value, bool is_set): value_(value), is_set_(is_set) {}

    OptionalT(): is_set_(false) {}

    const T& value() const {
        return value_;
    }

    bool is_set() const {
        return is_set_;
    }

    operator bool() const {
        return is_set_;
    }

    const T* operator->() const {
        return &value_;
    }

    const T& operator*() const {
        return value_;
    }
};


template <typename T>
using Optional = boost::optional<T>;

template <typename T>
std::ostream& operator<<(std::ostream& out, const OptionalT<T>& op) {
    if (op) {
        out<<op.value();
    }
    else {
        out<<"[none]";
    }
    return out;
}

template <typename T>
struct HasFieldFactory<OptionalT<T>>: HasFieldFactory<T> {};

template <typename T>
struct HasValueCodec<OptionalT<T>>: HasValueCodec<T> {};


template <typename T>
struct HasFieldFactory<Optional<T>> : HasFieldFactory<T> {};

template <typename T>
struct HasValueCodec<Optional<T>> : HasValueCodec<T> {};



}}
