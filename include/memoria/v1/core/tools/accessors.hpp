
// Copyright 2013 Victor Smirnov
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

#include <memoria/v1/core/tools/bitmap.hpp>

#include <functional>

namespace memoria {
namespace v1 {

template <typename T, typename V, int32_t BitsPerElement> class BitmapAccessor;

template <typename T, typename V, int32_t BitsPerElement>
class BitmapAccessor<T*, V, BitsPerElement> {
    T* values_;
    int32_t idx_;

public:
    BitmapAccessor(T* values, int32_t idx): values_(values), idx_(idx) {}

    V value() const {
        return GetBits(values_, idx_ * BitsPerElement, BitsPerElement);
    }

    operator V() const {
        return value();
    }

    V operator=(const V& value)
    {
        SetBits(values_, idx_ * BitsPerElement, value, BitsPerElement);
        return value;
    }
};

template <typename T, typename V, int32_t BitsPerElement>
class BitmapAccessor<const T*, V, BitsPerElement> {
    const T* values_;
    int32_t idx_;
public:
    BitmapAccessor(const T* values, int32_t idx): values_(values), idx_(idx) {}

    V value() const {
        return GetBits(values_, idx_ * BitsPerElement, BitsPerElement);
    }

    operator V() const {
        return value();
    }
};



template <typename V>
class FnAccessor {
public:

    typedef std::function <void (const V&)>  Setter;
    typedef std::function <V ()>             Getter;

private:
    Getter getter_fn_;
    Setter setter_fn_;

public:
    FnAccessor(Getter getter, Setter setter): getter_fn_(getter), setter_fn_(setter) {}

    V value() const {
        return getter_fn_();
    }

    operator V() const {
        return value();
    }

    V operator=(const V& value)
    {
        setter_fn_(value);
        return value;
    }

    V operator+=(const V& value)
    {
        V v = this->value();

        setter_fn_(v + value);
        return v + value;
    }

    V operator-=(const V& value)
    {
        V v = this->value();

        setter_fn_(v - value);
        return v - value;
    }
};


template <typename V>
class VLEFnAccessor: public FnAccessor<V> {

    typedef FnAccessor<V> Base;

public:
    typedef std::function <size_t ()>                LengthGetter;

private:

    LengthGetter length_getter_;

public:
    VLEFnAccessor(typename Base::Getter getter, typename Base::Setter setter, LengthGetter length_getter):
        Base(getter, setter), length_getter_(length_getter)
    {}

    size_t length() const {
        return length_getter_();
    }
};


template <typename V>
class ConstFnAccessor {
public:

    typedef std::function <V ()>             Getter;

private:
    Getter getter_fn_;


public:
    ConstFnAccessor(Getter getter): getter_fn_(getter) {}

    V value() const {
        return getter_fn_();
    }

    operator V() const {
        return value();
    }
};



template <typename V>
class ConstVLEFnAccessor: public ConstFnAccessor<V> {

    typedef FnAccessor<V> Base;

public:
    typedef std::function <size_t ()>                LengthGetter;

private:

    LengthGetter length_getter_;

public:
    ConstVLEFnAccessor(typename Base::Getter getter, LengthGetter length_getter):
        Base(getter), length_getter_(length_getter)
    {}

    size_t length() const {
        return length_getter_();
    }
};



}}
