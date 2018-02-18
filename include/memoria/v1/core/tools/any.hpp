
// Copyright 2011 Victor Smirnov
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

#include <typeinfo>

namespace memoria {
namespace v1 {

template <typename> struct Any2Type;



const char* ExtractMemoriaPath(const char*);






struct Any {
    enum Type {UNDEFINED, BYTE, UBYTE, SHORT, USHORT, INT, UINT, BIGINT, UBIGINT, FLOAT, DOUBLE, LDOUBLE, PCHAR};
private:

    Type type_;

    union {
        int8_t        b_value;
        uint8_t       ub_value;
        int16_t       s_value;
        uint16_t      us_value;
        int32_t         i_value;
        uint32_t        ui_value;
        int64_t      bi_value;
        uint64_t     ubi_value;
        float       f_value;
        double      d_value;
        long double ld_value;

        const char* pc_value;



        operator int8_t() const {
            return b_value;
        }
        operator uint8_t() const {
            return ub_value;
        }

        operator int16_t() const {
            return s_value;
        }
        operator uint16_t() const {
            return us_value;
        }

        operator int32_t() const {
            return i_value;
        }
        operator uint32_t() const {
            return ui_value;
        }

        operator int64_t() const {
            return bi_value;
        }
        operator uint64_t() const {
            return ubi_value;
        }

        operator float() const {
            return f_value;
        }
        operator double() const {
            return d_value;
        }
        operator long double() const {
            return ld_value;
        }

        operator const char*() const {
            return pc_value;
        }

        void operator=(int8_t value) {
            b_value = value;
        }
        void operator=(uint8_t value) {
            ub_value = value;
        }

        void operator=(int16_t value) {
            s_value = value;
        }
        void operator=(uint16_t value) {
            us_value = value;
        }

        void operator=(int32_t value) {
            i_value = value;
        }
        void operator=(uint32_t value) {
            ui_value = value;
        }

        void operator=(int64_t value) {
            bi_value = value;
        }
        void operator=(uint64_t value) {
            ubi_value = value;
        }

        void operator=(float value) {
            f_value = value;
        }

        void operator=(double value) {
            d_value = value;
        }

        void operator=(long double value) {
            ld_value = value;
        }

        void operator=(const char* value) {
            pc_value = value;
        }

    } value_;

public:



    Any(): type_(UNDEFINED) {}

    Any(int8_t value): type_(BYTE) {value_.b_value = value;}
    Any(uint8_t value): type_(UBYTE) {value_.ub_value = value;}
    Any(int16_t value): type_(SHORT) {value_.s_value = value;}
    Any(uint16_t value): type_(USHORT) {value_.us_value = value;}
    Any(int32_t value): type_(INT) {value_.i_value = value;}
    Any(uint32_t value): type_(UINT) {value_.ui_value = value;}
    Any(int64_t value): type_(BIGINT) {value_.bi_value = value;}
    Any(uint64_t value): type_(UBIGINT) {value_.ubi_value = value;}
    Any(float value): type_(FLOAT) {value_.f_value = value;}
    Any(double value): type_(DOUBLE) {value_.d_value = value;}
    Any(long double value): type_(LDOUBLE) {value_.ld_value = value;}
    Any(const char* value): type_(PCHAR) {value_.pc_value = value;}

    template <typename T>
    operator T() const
    {
        if (type_ == Any2Type<T>::Value)
        {
            return (T)value_;
        }
        else {
            throw Exception(MMA1_SOURCE, SBuf()<<typeid(T).name()<<" type="<<type_);
        }
    }

    template <typename T>
    Any& operator=(const T& value)
    {
        type_   = (Type)Any2Type<T>::Value;
        value_  = value;

        return *this;
    }

    Type type() const {
        return type_;
    }
};



template<> struct Any2Type<int8_t>: TypeCode<Any::BYTE>       {};
template<> struct Any2Type<uint8_t>: TypeCode<Any::UBYTE>     {};

template<> struct Any2Type<int16_t>: TypeCode<Any::SHORT>     {};
template<> struct Any2Type<uint16_t>: TypeCode<Any::USHORT>   {};

template<> struct Any2Type<int32_t>: TypeCode<Any::INT>         {};
template<> struct Any2Type<uint32_t>: TypeCode<Any::UINT>       {};

template<> struct Any2Type<int64_t>: TypeCode<Any::BIGINT>   {};
template<> struct Any2Type<uint64_t>: TypeCode<Any::UBIGINT> {};

template<> struct Any2Type<float>: TypeCode<Any::FLOAT> {};
template<> struct Any2Type<double>: TypeCode<Any::DOUBLE> {};
template<> struct Any2Type<long double>: TypeCode<Any::LDOUBLE> {};

template<> struct Any2Type<const char*>: TypeCode<Any::PCHAR> {};

}

namespace std {

using namespace memoria;

ostream& operator<<(ostream& out, const Any& any);

}}