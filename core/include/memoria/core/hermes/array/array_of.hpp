
// Copyright 2023 Victor Smirnov
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

#include <memoria/core/hermes/array/object_array.hpp>
#include <memoria/core/hermes/array/typed_array.hpp>


namespace memoria::hermes {

template <typename T>
class ArrayOf {
    ObjectArray array_;

public:
    struct iterator
    {
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;  // or also value_type*
        using reference         = T&;  // or also value_type&
    private:
        using ArrayIterator = typename ObjectArray::iterator;

        ArrayIterator iter_;

    public:
        iterator(ArrayIterator iter):
            iter_(iter)
        {}

        value_type operator*() const {
            return T(*iter_);
        }

        // Prefix increment
        iterator& operator++() {
            iter_++;
            return *this;
        }

        // Postfix increment
        iterator operator++(int)
        {
            iterator tmp = *this; ++(*this);
            return tmp;
        }

        iterator operator+=(size_t n) {
            iter_ += n;
            return *this;
        }

        iterator operator-=(size_t n) {
            iter_ -= n;
            return *this;
        }

        friend bool operator== (const iterator& a, const iterator& b) {
            return a.iter_ == b.iter_;
        }

        friend bool operator!= (const iterator& a, const iterator& b) {
            return a.iter_ != b.iter_;
        }
    };

    using const_iterator = iterator;

    ArrayOf() {}
    ArrayOf(ObjectArray array):
        array_(array)
    {}

    iterator begin() const {}
    iterator end() const {}

    size_t size() const {
        return !array_.is_null() ? array_.size() : 0;
    }

    T operator[](size_t idx) const {
        return T{ array_.get(idx) };
    }

};


}
