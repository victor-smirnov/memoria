
// Copyright 2011 Victor Smirnov, Ivan Yurchenko
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
#include <memoria/v1/core/types/type2type.hpp>

#include <memoria/v1/core/strings/string.hpp>

namespace memoria {
namespace v1 {

template <typename T> class PageID;



class IDValue {
    int8_t data_[8];
public:
    IDValue() {
        clear();
    }

    template <typename T>
    IDValue(const T* id) {
        clear();
        id->copyTo(ptr());
    }

    IDValue(U8StringRef id) {
        clear();
    }

    IDValue(int64_t id)
    {
        int64_t* data_ptr = T2T<int64_t*>(data_);
        *data_ptr = id;
    }

    template <typename T>
    IDValue(const PageID<T>& id)
    {
        clear();
        id.copyTo(ptr());
    }

    template <typename T>
    IDValue& operator=(const PageID<T>& id)
    {
        clear();
        id.copyTo(ptr());
        return *this;
    }


    void clear() {
        for (unsigned c = 0; c < sizeof(data_); c++) {
            data_[c] = 0;
        }
    }

    template <typename T>
    void set(const T& id) {
        clear();
        id.copyTo(ptr());
    }

    template <typename T>
    const T get() const {
        return T(*this);
    }

    const void* ptr() const {
        return data_;
    }

    void* ptr() {
        return data_;
    }

    virtual const U8String str() const
    {
        char text[sizeof(data_)*2 + 5];
        text[0] = '[';
        text[1] = '0';
        text[2] = 'x';
        text[sizeof(text) - 2] = ']';
        text[sizeof(text) - 1] = 0;

        for (unsigned c = 0; c < sizeof(data_); c++) {
            text[c*2 + 4] = get_char(data_[sizeof(data_) - c - 1] & 0xf);
            text[c*2 + 3] = get_char((data_[sizeof(data_) - c - 1] >> 4) & 0xf);
        }

        return U8String(text);
    }

    virtual bool isNull() const {
        for (unsigned c = 0; c < sizeof(data_); c++) {
            if (data_[c] != 0) {
                return false;
            }
        }
        return true;
    }

    bool equals(const IDValue& other) const {
        for (unsigned c = 0; c < sizeof(data_); c++) {
            if (data_[c] != other.data_[c]) {
                return false;
            }
        }
        return true;
    }

    bool operator==(const IDValue& other) const {
        return equals(other);
    }

    bool operator<(const IDValue& other) const
    {
        for (int32_t c = 7; c >=0; c--)
        {
            int32_t v0 = data_[c];
            int32_t v1 = other.data_[c];
            if (v0 != v1)
            {
                return v0 < v1;
            }
        }
        return false;
    }

private:

    static char get_char(int8_t value) {
        char chars[] = {
            '0', '1', '2', '3',
            '4', '5', '6', '7',
            '8', '9', 'a', 'b',
            'c', 'd', 'e', 'f'};
        if (value >= 0 && value <= 15) {
            return chars[(int)value];
        }
        return 'X';
    }
};


std::ostream& operator<<(std::ostream& os, const IDValue& id);

}}
