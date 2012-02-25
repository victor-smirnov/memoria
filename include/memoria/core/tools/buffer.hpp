
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#ifndef _MEMORIA_CORE_TOOLS_BUFFER_H
#define	_MEMORIA_CORE_TOOLS_BUFFER_H


#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/bitmap.hpp>
#include <memoria/core/types/typehash.hpp>

#include <iostream>



namespace memoria    {

#pragma pack(1)

typedef long bitmap_atom_t;

template <long Size, long Align = sizeof(long)>
class Padding {
    static const long SIZE = (sizeof(Size) % Align == 0 ? Align : (sizeof(bitmap_atom_t) - Size % sizeof(bitmap_atom_t))) ;
    char bytes[SIZE];
public:
    Padding(){}
};

template <typename T>
class Wrapper {
    T value_;
public:
    static const int SIZE                           = sizeof(T);
    static const int BITSIZE                        = sizeof(T) * 8;

    typedef T                                       ValueType;

    Wrapper(const T &value): value_(value) {};
    Wrapper(const Wrapper<T> &copy): value_(copy.value_) {};

    T &value() const {
        return value_;
    }

    void set_value(const T &v) {
        value_ = v;
    }

    bool operator==(const Wrapper<T> &v) const {
        return value_ == v.value_;
    }

    bool operator!=(const Wrapper<T> &v) const {
        return value_ != v.value_;
    }

    bool operator>=(const Wrapper<T> &v) const {
        return value_ >= v.value_;
    }

    bool operator<=(const Wrapper<T> &v) const {
        return value_ <= v.value_;
    }

    bool operator<(const Wrapper<T> &v) const {
        return value_ < v.value_;
    }
};

template <size_t Size>
class Buffer {
    char Buffer_[Size];

    typedef Buffer<Size> Me;

public:
    typedef Long          	Element;
    
    static const BigInt     SIZE 	= Size;                 //in bytes;
    static const BigInt     BITSIZE = SIZE * 8;          	//in bits;

    Buffer() {}

    const Me& operator=(const Me& other) {
    	CopyBuffer(other.Buffer_, Buffer_, Size);
        return *this;
    }

    bool operator==(const Me&other) const {
    	return CompareBuffers(Buffer_, other.Buffer_, Size);
    }

    bool operator!=(const Me&other) const {
    	return !CompareBuffers(Buffer_, other.Buffer_, Size);
    }

    const char *ptr() const {
        return Buffer_;
    }

    char *ptr() {
        return Buffer_;
    }

    void Clear() {
        for (Int c = 0; c < (Int)Size; c++) {
            Buffer_[c] = 0;
        }
    }

    static bool IsVoid() {
        return SIZE == 0;
    }

    const Long &operator[](Int idx) const {
        return *(CP2CP<Long>(ptr()) + idx);
    }

    Long &operator[](Int idx) {
        return *(T2T<Long*>(ptr()) + idx);
    }

    void CopyFrom(const void *mem) {
    	CopyBuffer(mem, Buffer_, Size);
    }

    void CopyTo(void *mem) const {
        CopyBuffer(Buffer_, mem, Size);
    }

    void dump(std::ostream &os, Int size = BITSIZE) {
        Dump(os, *this, (Int)0, size);
    }

    Int GetHashCode() {
        return PtrToInt(ptr());
    }
};


template <typename Object, size_t Size = sizeof(Object)>
class ValueBuffer /*: public Buffer<Size> */{
//    typedef Buffer<Size>                        Base;
    typedef ValueBuffer<Object, Size>           Me;
    Object value_;

public:

    static const BigInt     SIZE 	= Size;                 //in bytes;
    static const BigInt     BITSIZE = SIZE * 8;          	//in bits;

    typedef Object                              ValueType;

    ValueBuffer() {}

    ValueBuffer(const Object &obj) {
        value() = obj;
    }

    void Clear() {
    	value_ = 0;
    }

    void CopyTo(void *mem) const {
    	CopyBuffer(&value_, mem, Size);
    }

    void CopyFrom(const void *mem) {
    	CopyBuffer(mem, &value_, Size);
    }

    const Object &value() const {
        return value_;//P2CR<Object>(Base::ptr());
    }

    Object &value() {
        return value_;//P2R<Object>(Base::ptr());
    }


    bool operator<(const Me &other) const {
        return value() < other.value();
    }

    Int GetHashCode() {
        return CShr(PtrToInt(&value_), 3) /*^ memoria::tools::types::TypeHash<Int>::Value*/;
    }
};

class VoidBuffer {
public:

    static const int SIZE = 0;              //in bytes;
    static const int BITSIZE = SIZE * 8;    //in bits;

    VoidBuffer() {}

    static bool is_void() {
        return true;
    }

    const char* ptr() const {
        return NULL;
    }

    char* ptr() {
        return NULL;
    }

    Int GetHashCode() {
        return 0;
    }
};

#pragma pack()

} //memoria



#endif
