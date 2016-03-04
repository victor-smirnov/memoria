
// Copyright Victor Smirnov, Ivan Yurchenko 2012-2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_vctr_HPP
#define _MEMORIA_CORE_TOOLS_vctr_HPP


#include <memoria/core/types/types.hpp>
#include <memoria/core/types/type2type.hpp>

#include <memoria/core/exceptions/bounds.hpp>

#include <algorithm>
#include <type_traits>

namespace memoria    {
namespace core       {


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

template <typename Value, Int Size = 16, typename ClearingFunctor = EmptyValueFunctor>
class StaticArray{

    typedef StaticArray<Value, Size, ClearingFunctor> MyType;

    Int     size_;
    Value   values_[Size];

public:
    typedef Value Element;

    StaticArray(): size_(0) {}

    StaticArray(Int size): size_(size)
    {
        for (Int c = 0; c < Size; c++)
        {
            functor(values_[c]);
        }
    }

    StaticArray(const MyType& other)
    {
        size_ = other.size_;

        for (Int c = 0; c < Size; c++)
        {
            values_[c] = other.values_[c];
        }
    }

    StaticArray(MyType&& other)
    {
        size_ = other.size_;

        for (Int c = 0; c < Size; c++)
        {
            values_[c] = std::move(other.values_[c]);
        }
    }

    MyType& operator=(const MyType& other)
    {
        size_ = other.size_;

        for (Int c = 0; c < size_; c++)
        {
            values_[c] = other.values_[c];
        }

        ClearingFunctor functor;

        for (Int c = size_; c < Size; c++)
        {
            functor(values_[c]);
        }

        return *this;
    }

    MyType& operator=(MyType&& other)
    {
        size_ = other.size_;

        for (Int c = 0; c < size_; c++)
        {
            values_[c] = std::move(other.values_[c]);
        }

        ClearingFunctor functor;

        for (Int c = size_; c < Size; c++)
        {
            functor(values_[c]);
        }

        return *this;
    }

    const Value& operator[](Int idx) const {
        return values_[idx];
    }

    Value& operator[](Int idx) {
        return values_[idx];
    }

    Int getSize() const {
        return size_;
    }

    Int size() const {
    	return size_;
    }

    Int capacity() const {
        return Size - size_;
    }

    void resize(Int size)
    {
        size_ = size;
    }

    static Int getMaxSize() {
        return Size;
    }

    void insert(Int idx, const Value& value)
    {
        for (Int c = size_; c > idx; c--)
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

    void remove(Int idx)
    {
        for (Int c = idx; c < size_; c++)
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

        for (Int c = 0; c < size_; c++)
        {
            functor(values_[c]);
        }

        size_ = 0;
    }
};







template <typename ElementType_, Int Indexes_>
class StaticVector
{
    typedef StaticVector<ElementType_, Indexes_> MyType;

    ElementType_ values_[Indexes_];

    template <typename, Int>
    friend class StaticVector;

public:
    typedef ElementType_ ElementType;


    static const Int Indexes = Indexes_;

    StaticVector()
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = ElementType_();
        }
    }

    explicit StaticVector(const ElementType& value)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = value;
        }
    }



    template <typename T>
    StaticVector(std::initializer_list<T> list)
    {
    	T last = T();

        Int idx = 0;
        for (const T& e: list)
        {
            last = values_[idx++] = e;
        }

        for (Int c = idx; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c];
        }
    }

    static MyType create(Int idx, const ElementType& value)
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

    const ElementType& value(Int idx) const
    {
    	check(idx);
        return values_[idx];
    }

    ElementType& value(Int idx)
    {
    	check(idx);
        return values_[idx];
    }

    const ElementType& operator[](Int idx) const
    {
    	check(idx);
        return values_[idx];
    }

    ElementType& operator[](Int idx)
    {
    	check(idx);
    	return values_[idx];
    }

    void clear()
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = 0;
        }
    }

    template <typename T, Int TIndexes, typename = std::enable_if<TIndexes <= Indexes>>
    MyType& assignUp(const StaticVector<T, TIndexes>& other)
    {
        Int shift = Indexes - TIndexes;

        for (Int c = Indexes - 1; c >= shift ; c--)
        {
            values_[c] = other.values_[c - shift];
        }

        return *this;
    }

    template <typename T, Int TIndexes, typename = std::enable_if<TIndexes >= Indexes>>
    MyType& assignDown(const StaticVector<T, TIndexes>& other)
    {
        Int shift = TIndexes - Indexes;

        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c + shift];
        }

        return *this;
    }

    template <typename T, Int TIndexes, typename = std::enable_if<TIndexes <= Indexes>>
    MyType& sumUp(const StaticVector<T, TIndexes>& other)
    {
        Int shift = Indexes - TIndexes;

        for (Int c = Indexes - 1; c >= shift ; c--)
        {
            values_[c] += other.values_[c - shift];
        }

        return *this;
    }

    template <typename T, Int TIndexes, typename = std::enable_if<TIndexes <= Indexes>>
    MyType& sumAt(Int idx, const StaticVector<T, TIndexes>& other)
    {
        for (Int c = 0; c < TIndexes; c++)
        {
            values_[c + idx] += other.values_[c];
        }

        return *this;
    }

    bool operator==(const MyType& other) const
    {
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0, mask = 1; c < Indexes; c++, mask <<= 1)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
        {
            if (values_[c] < other.values_[c])
            {
                return true;
            }
        }

        return false;
    }

    bool gtAny( const MyType& other ) const
    {
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
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
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = other.values_[c];
        }

        return *this;
    }

    template <typename Value>
    MyType& operator=(const Value& value)
    {
    	for (Int c = 0; c < Indexes; c++) values_[c] = value;
    	return *this;
    }


    template <typename Value>
    MyType& operator=(const std::initializer_list<Value>& list)
    {
    	Int idx = 0;
    	for (Value e: list)
    	{
    		values_[idx++] = e;

    		if (idx >= Indexes)
    		{
    			break;
    		}
    	}

    	for (Int c = idx; c < Indexes; c++) {
    		values_[c] = ElementType_();
    	}

    	return *this;
    }



//    MyType& operator=(const ElementType* keys)
//    {
//        for (Int c = 0; c < Indexes; c++)
//        {
//            values_[c] = keys[c];
//        }
//        return *this;
//    }

    MyType& setAll(const ElementType& keys)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] = keys;
        }
        return *this;
    }

    template <typename T>
    MyType& operator+=(const StaticVector<T, Indexes>& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] += other.values_[c];
        }

        return *this;
    }

    MyType& operator+=(const ElementType& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] += other;
        }

        return *this;
    }

    MyType operator+(const MyType& other) const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] += other.values_[c];
        }

        return result;
    }


    MyType operator-(const MyType& other) const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] -= other.values_[c];
        }

        return result;
    }

    MyType operator-() const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] = -values_[c];
        }

        return result;
    }


    template <typename T>
    MyType& operator-=(const StaticVector<T, Indexes>& other)
    {
        for (Int c = 0; c < Indexes; c++)
        {
            values_[c] -= other.values_[c];
        }

        return *this;
    }

    MyType operator/(const ElementType& divisor) const
    {
        MyType result = *this;

        for (Int c = 0; c < Indexes; c++)
        {
            result.values_[c] = values_[c] / divisor;
        }

        return result;
    }

    UBigInt gtZero() const
    {
        UBigInt result = 0;

        for (Int c = 0; c < Indexes; c++)
        {
            result += (UBigInt(values_[c] > 0)) << c;
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

    	for (Int c = 1; c < Indexes; c++)
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

    	for (Int c = 1; c < Indexes; c++)
    	{
    		if (values_[c] < value)
    		{
    			value = values_[c];
    		}
    	}

    	return value;
    }

private:
    static void check(Int idx)
    {
//    	if (idx < 0 || idx >= Indexes_) {
//    		throw BoundsException(MEMORIA_SOURCE, SBuf()<<"Invalid StaticVector index: "<<idx);
//    	}
    }
};


template <typename K, typename... Args>
auto make_sv(Args&&... values) -> StaticVector<K, sizeof...(Args)>{
	return StaticVector<K, sizeof...(Args)>({values...});
}


template <typename T, typename... Args>
auto MakeStaticVector(Args&&... args) -> StaticVector<T, sizeof...(Args)>
{
	return StaticVector<T, sizeof...(Args)>(std::forward<Args>(args)...);
}


template <typename Key, memoria::Int Indexes>
std::ostream& operator<<(std::ostream& out, const ::memoria::core::StaticVector<Key, Indexes>& accum)
{
    out<<"[";

    for (memoria::Int c = 0; c < Indexes; c++)
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


template <typename T, Int Size>
struct FromString<core::StaticVector<T, Size>> {
    static void convert(core::StaticVector<T, Size>& values, String str)
    {
        Int start = 0;

        for (size_t c = 0; c < Size; c++)
        {
            values[c] = 0;
        }

        for (Int c = str.size() - 1; c >= 0; c--)
        {
        	if (str[c] == '[' || str[c] == ']') {
        		str.erase(c, 1);
        	}
        }

        for (Int c = 0; c < Size; c++)
        {
            size_t pos = str.find_first_of(",", start);

            String value = trimString(str.substr(start, pos != String::npos ? pos - start : pos));

            if (!isEmpty(value))
            {
                values[c] = FromString<T>::convert(value);
            }
            else {
                values[c] = 0;
            }

            if (pos != String::npos && pos < str.length())
            {
                start = pos + 1;
            }
            else {
                break;
            }
        }
    }
};



}
#endif

