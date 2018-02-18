
// Copyright 2012 Victor Smirnov, Ivan Yurchenko
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


#include <memoria/v1/core/config.hpp>
#include <memoria/v1/core/types/type2type.hpp>
#include <memoria/v1/core/tools/optional.hpp>
#include <memoria/v1/core/strings/strings.hpp>

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <algorithm>
#include <type_traits>

namespace memoria {
namespace v1 {
namespace core {


struct EmptyValueFunctor {

    template <typename Value>
    void operator()(Value&) {}
};


struct NullPtrFunctor {

    template <typename Value>
    void operator()(Value& val)
    {
        val = nullptr;
    }
};

template <typename Value, int32_t Size = 16, typename ClearingFunctor = EmptyValueFunctor>
class StaticArray{

    typedef StaticArray<Value, Size, ClearingFunctor> MyType;

    int32_t     size_;
    Value   values_[Size];

public:
    typedef Value Element;

    StaticArray(): size_(0) {}

    StaticArray(int32_t size): size_(size)
    {
        for (int32_t c = 0; c < Size; c++)
        {
            functor(values_[c]);
        }
    }

    StaticArray(const MyType& other)
    {
        size_ = other.size_;

        for (int32_t c = 0; c < Size; c++)
        {
            values_[c] = other.values_[c];
        }
    }

    StaticArray(MyType&& other)
    {
        size_ = other.size_;

        for (int32_t c = 0; c < Size; c++)
        {
            values_[c] = std::move(other.values_[c]);
        }
    }

    MyType& operator=(const MyType& other)
    {
        size_ = other.size_;

        for (int32_t c = 0; c < size_; c++)
        {
            values_[c] = other.values_[c];
        }

        ClearingFunctor functor;

        for (int32_t c = size_; c < Size; c++)
        {
            functor(values_[c]);
        }

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        size_ = other.size_;

        for (int32_t c = 0; c < size_; c++)
        {
            values_[c] = std::move(other.values_[c]);
        }

        ClearingFunctor functor;

        for (int32_t c = size_; c < Size; c++)
        {
            functor(values_[c]);
        }

        return *this;
    }

    const Value& operator[](int32_t idx) const {
        return values_[idx];
    }

    Value& operator[](int32_t idx) {
        return values_[idx];
    }

    int32_t getSize() const {
        return size_;
    }

    int32_t size() const {
        return size_;
    }

    int32_t capacity() const {
        return Size - size_;
    }

    void resize(int32_t size)
    {
        size_ = size;
    }

    static int32_t getMaxSize() {
        return Size;
    }

    void insert(int32_t idx, const Value& value)
    {
        for (int32_t c = size_; c > idx; c--)
        {
            values_[c] = values_[c - 1];
        }

        values_[idx] = value;
        size_++;
    }

    void append(const Value& value)
    {
        values_[size_++] = value;
    }

    void remove(int32_t idx)
    {
        for (int32_t c = idx; c < size_; c++)
        {
            values_[c] = values_[c + 1];
        }

        ClearingFunctor functor;

        size_--;

        functor(values_[size_]);
    }

    void removeLast()
    {
        remove(getSize() - 1);
    }

    void clear()
    {
        ClearingFunctor functor;

        for (int32_t c = 0; c < size_; c++)
        {
            functor(values_[c]);
        }

        size_ = 0;
    }
};







template <typename ElementType_, int32_t Indexes_>
class StaticVector
{
    typedef StaticVector<ElementType_, Indexes_> MyType;

    ElementType_ values_[Indexes_ > 0 ? Indexes_ : 1];

    template <typename, int32_t>
    friend class StaticVector;

public:
    typedef ElementType_ ElementType;


    static const int32_t Indexes = Indexes_;

    StaticVector()
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = ElementType_();
        }
    }

    explicit StaticVector(const ElementType& value)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = value;
        }
    }



    template <typename T>
    StaticVector(std::initializer_list<T> list)
    {
        T last = T();

        int32_t idx = 0;
        for (const T& e: list)
        {
            last = values_[idx++] = e;
        }

        for (int32_t c = idx; c < Indexes; c++)
        {
            values_[c] = last;
        }
    }

    ElementType get() const
    {
        check(0);
        return values_[0];
    }

    StaticVector(const MyType& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c];
        }
    }

    StaticVector(MyType&& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = std::move(other.values_[c]);
        }
    }

    static MyType create(int32_t idx, const ElementType& value)
    {
        check(idx);

        MyType me;

        me[idx] = value;

        return me;
    }


    const ElementType* values() const
    {
        return values_;
    }

    ElementType* values()
    {
        return values_;
    }

    const ElementType& value(int32_t idx) const
    {
        check(idx);
        return values_[idx];
    }

    ElementType& value(int32_t idx)
    {
        check(idx);
        return values_[idx];
    }

    const ElementType& operator[](int32_t idx) const
    {
        check(idx);
        return values_[idx];
    }

    ElementType& operator[](int32_t idx)
    {
        check(idx);
        return values_[idx];
    }

    void clear()
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = 0;
        }
    }

    template <typename T, int32_t TIndexes, typename = std::enable_if<TIndexes <= Indexes>>
    MyType& assignUp(const StaticVector<T, TIndexes>& other)
    {
        int32_t shift = Indexes - TIndexes;

        for (int32_t c = Indexes - 1; c >= shift ; c--)
        {
            values_[c] = other.values_[c - shift];
        }

        return *this;
    }

    template <typename T, int32_t TIndexes, typename = std::enable_if<TIndexes >= Indexes>>
    MyType& assignDown(const StaticVector<T, TIndexes>& other)
    {
        int32_t shift = TIndexes - Indexes;

        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c + shift];
        }

        return *this;
    }

    template <typename T, int32_t TIndexes, typename = std::enable_if<TIndexes <= Indexes>>
    MyType& sumUp(const StaticVector<T, TIndexes>& other)
    {
        int32_t shift = Indexes - TIndexes;

        for (int32_t c = Indexes - 1; c >= shift ; c--)
        {
            values_[c] += other.values_[c - shift];
        }

        return *this;
    }

    template <typename T, int32_t TIndexes, typename = std::enable_if<TIndexes <= Indexes>>
    MyType& sumAt(int32_t idx, const StaticVector<T, TIndexes>& other)
    {
        for (int32_t c = 0; c < TIndexes; c++)
        {
            values_[c + idx] += other.values_[c];
        }

        return *this;
    }

    bool operator==(const MyType& other) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] != other.values_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const MyType& other) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] != other.values_[c])
            {
                return true;
            }
        }

        return false;
    }

    bool operator<=(const MyType& other) const
    {
        for (int32_t c = 0, mask = 1; c < Indexes; c++, mask <<= 1)
        {
            bool set = 1;

            if (set && values_[c] > other.values_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool gteAll( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] < other.values_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool lteAll( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] > other.values_[c])
            {
                return false;
            }
        }

        return true;
    }


    bool ltAll( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] >= other.values_[c])
            {
                return false;
            }
        }

        return true;
    }


    bool gteAll(const ElementType& other) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] < other)
            {
                return false;
            }
        }

        return true;
    }

    bool lteAll(const ElementType& other) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] > other)
            {
                return false;
            }
        }

        return true;
    }

    bool ltAny( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] < other.values_[c])
            {
                return true;
            }
        }

        return false;
    }

    bool ltAny( const ElementType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] < other)
            {
                return true;
            }
        }

        return false;
    }

    bool gtAny( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] > other.values_[c])
            {
                return true;
            }
        }

        return false;
    }

    bool gtAny( const ElementType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] > other)
            {
                return true;
            }
        }

        return false;
    }

    bool gtAll( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] <= other[c])
            {
                return false;
            }
        }

        return true;
    }

    bool gtAll( const ElementType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] <= other)
            {
                return false;
            }
        }

        return true;
    }


    bool eqAll( const MyType& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] != other.values_[c])
            {
                return false;
            }
        }

        return true;
    }

    bool eqAll( const ElementType_& other ) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] != other)
            {
                return false;
            }
        }

        return true;
    }

    bool operator>(const MyType& other) const
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            if (values_[c] <= other.values_[c])
            {
                return false;
            }
        }

        return true;
    }


    MyType& operator=(const MyType& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c];
        }

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = std::move(other.values_[c]);
        }

        return *this;
    }

    template <typename Value>
    MyType& operator=(const Value& value)
    {
        for (int32_t c = 0; c < Indexes; c++) values_[c] = value;
        return *this;
    }

    template <typename Value>
    MyType& operator=(const Optional<Value>& value)
    {
        if (value.is_set()) {
            for (int32_t c = 0; c < Indexes; c++) values_[c] = value.value();
        }
        else {
            for (int32_t c = 0; c < Indexes; c++) values_[c] = Value();
        }

        return *this;
    }

    template <typename Value>
    MyType& operator=(const StaticVector<Optional<Value>, Indexes>& value)
    {
        for (int32_t c = 0; c < Indexes; c++) {
            if (value[c].is_set()) {
                values_[c] = value[c].value();
            }
            else {
                values_[c] = Value();
            }
        }

        return *this;
    }




    template <typename Value>
    MyType& operator=(const std::initializer_list<Value>& list)
    {
        int32_t idx = 0;
        for (Value e: list)
        {
            values_[idx++] = e;

            if (idx >= Indexes)
            {
                break;
            }
        }

        for (int32_t c = idx; c < Indexes; c++) {
            values_[c] = ElementType_();
        }

        return *this;
    }



//    MyType& operator=(const ElementType* keys)
//    {
//        for (int32_t c = 0; c < Indexes; c++)
//        {
//            values_[c] = keys[c];
//        }
//        return *this;
//    }

    MyType& setAll(const ElementType& keys)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] = keys;
        }
        return *this;
    }

    template <typename T>
    MyType& operator+=(const StaticVector<T, Indexes>& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] += other.values_[c];
        }

        return *this;
    }

    MyType& operator+=(const ElementType& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] += other;
        }

        return *this;
    }

    MyType operator+(const MyType& other) const
    {
        MyType result = *this;

        for (int32_t c = 0; c < Indexes; c++)
        {
            result.values_[c] += other.values_[c];
        }

        return result;
    }


    MyType operator-(const MyType& other) const
    {
        MyType result = *this;

        for (int32_t c = 0; c < Indexes; c++)
        {
            result.values_[c] -= other.values_[c];
        }

        return result;
    }

    MyType operator-() const
    {
        MyType result = *this;

        for (int32_t c = 0; c < Indexes; c++)
        {
            result.values_[c] = -values_[c];
        }

        return result;
    }


    template <typename T>
    MyType& operator-=(const StaticVector<T, Indexes>& other)
    {
        for (int32_t c = 0; c < Indexes; c++)
        {
            values_[c] -= other.values_[c];
        }

        return *this;
    }

    MyType operator/(const ElementType& divisor) const
    {
        MyType result = *this;

        for (int32_t c = 0; c < Indexes; c++)
        {
            result.values_[c] = values_[c] / divisor;
        }

        return result;
    }

    uint64_t gtZero() const
    {
        uint64_t result = 0;

        for (int32_t c = 0; c < Indexes; c++)
        {
            result += (uint64_t(values_[c] > 0)) << c;
        }

        return result;
    }

    ElementType sum() const
    {
        ElementType value = 0;

        for (const auto& v: values_) {
            value += v;
        }

        return value;
    }

    auto max() const
    {
        ElementType value = values_[0];

        for (int32_t c = 1; c < Indexes; c++)
        {
            if (values_[c] > value)
            {
                value = values_[c];
            }
        }

        return value;
    }

    auto min() const
    {
        ElementType value = values_[0];

        for (int32_t c = 1; c < Indexes; c++)
        {
            if (values_[c] < value)
            {
                value = values_[c];
            }
        }

        return value;
    }

private:
    static void check(int32_t idx)
    {
//      if (idx < 0 || idx >= Indexes_) {
//          throw BoundsException(MMA1_SOURCE, SBuf()<<"Invalid StaticVector index: "<<idx);
//      }
    }
};

template <typename T1, typename T2, int32_t Indexes>
void OptionalAssignmentHelper(StaticVector<Optional<T1>, Indexes>& v1, const StaticVector<T2, Indexes>& v2)
{
    for (int32_t c = 0; c < Indexes; c++)
    {
        v1[c] = Optional<T1>(v2[c]);
    }
}

template <typename T1, typename T2, int32_t Indexes>
void OptionalAssignmentHelper(StaticVector<T1, Indexes>& v1, const StaticVector<T2, Indexes>& v2)
{
    for (int32_t c = 0; c < Indexes; c++)
    {
        v1[c] = v2[c];
    }
}


template <typename K, typename... Args>
auto make_sv(Args&&... values) -> StaticVector<K, sizeof...(Args)>{
    return StaticVector<K, sizeof...(Args)>({values...});
}


template <typename T, typename... Args>
auto MakeStaticVector(Args&&... args) -> StaticVector<T, sizeof...(Args)>
{
    return StaticVector<T, sizeof...(Args)>(std::forward<Args>(args)...);
}


template <typename Key, int32_t Indexes>
std::ostream& operator<<(std::ostream& out, const v1::core::StaticVector<Key, Indexes>& accum)
{
    out<<"[";

    for (int32_t c = 0; c < Indexes; c++)
    {
        out<<accum.value(c);

        if (c < Indexes - 1)
        {
            out<<", ";
        }
    }

    out<<"]";

    return out;
}




}


template <typename T> struct FromString;


template <typename T, int32_t Size>
struct FromString<core::StaticVector<T, Size>> {

    static void convert(core::StaticVector<T, Size>& values, U16String str) {
        convert(values, str.to_u8());
    }

    static void convert(core::StaticVector<T, Size>& values, U8String str)
    {
        int32_t start = 0;

        for (size_t c = 0; c < Size; c++)
        {
            values[c] = 0;
        }

        for (int32_t c = str.size() - 1; c >= 0; c--)
        {
            if (str[c] == '[' || str[c] == ']') {
                str.to_std_string().erase(c, 1);
            }
        }

        for (int32_t c = 0; c < Size; c++)
        {
            size_t pos = str.to_std_string().find_first_of(",", start);

            U8String value = trimString(str.to_std_string().substr(start, pos != StdString::npos ? pos - start : pos));

            if (!isEmpty(value))
            {
                values[c] = FromString<T>::convert(value);
            }
            else {
                values[c] = 0;
            }

            if (pos != StdString::npos && pos < str.length())
            {
                start = pos + 1;
            }
            else {
                break;
            }
        }
    }
};



}}
