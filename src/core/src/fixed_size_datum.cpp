
// Copyright 2019 Victor Smirnov
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

#include <memoria/v1/core/datatypes/datum.hpp>

namespace memoria {
namespace v1 {

template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, int64_t value) {
    return Datum<DataType>(value);
}

template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, double value) {
    return Datum<DataType>(value);
}

template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, bool value) {
    return Datum<DataType>(value);
}


template <typename DataType>
Datum<DataType> datum_from_sdn_value(const DataType*, const U8StringView& str)
{
    U8StringView tstr = trim_string(str);

    std::istringstream iss (tstr.to_string());

    using Value = typename DataTypeTraits<DataType>::ViewType;

    Value ivalue{};
    iss >> ivalue;

    if (!iss.eof()) {
        MMA1_THROW(RuntimeException()) << format_ex("Can't convert [{}] to {}", tstr, make_datatype_signature_string<BigInt>());
    }

    return Datum<DataType>(ivalue);
}


template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, const U8StringView&);
template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, int64_t);
template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, double);
template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, bool);

template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, const U8StringView&);
template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, int64_t);
template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, double);
template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, bool);

template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, const U8StringView&);
template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, int64_t);
template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, double);
template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, bool);

template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, const U8StringView&);
template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, int64_t);
template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, double);
template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, bool);


template Datum<Integer>     datum_from_sdn_value(const Integer*, const U8StringView&);
template Datum<Integer>     datum_from_sdn_value(const Integer*, int64_t);
template Datum<Integer>     datum_from_sdn_value(const Integer*, double);
template Datum<Integer>     datum_from_sdn_value(const Integer*, bool);

template Datum<UInteger>    datum_from_sdn_value(const UInteger*, const U8StringView&);
template Datum<UInteger>    datum_from_sdn_value(const UInteger*, int64_t);
template Datum<UInteger>    datum_from_sdn_value(const UInteger*, double);
template Datum<UInteger>    datum_from_sdn_value(const UInteger*, bool);

template Datum<BigInt>      datum_from_sdn_value(const BigInt*, const U8StringView&);
template Datum<BigInt>      datum_from_sdn_value(const BigInt*, int64_t);
template Datum<BigInt>      datum_from_sdn_value(const BigInt*, double);
template Datum<BigInt>      datum_from_sdn_value(const BigInt*, bool);


template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, const U8StringView&);
template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, int64_t);
template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, double);
template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, bool);

template Datum<Real>        datum_from_sdn_value(const Real*, const U8StringView&);
template Datum<Real>        datum_from_sdn_value(const Real*, int64_t);
template Datum<Real>        datum_from_sdn_value(const Real*, double);
template Datum<Real>        datum_from_sdn_value(const Real*, bool);

template Datum<Double>      datum_from_sdn_value(const Double*, const U8StringView&);
template Datum<Double>      datum_from_sdn_value(const Double*, int64_t);
template Datum<Double>      datum_from_sdn_value(const Double*, double);
template Datum<Double>      datum_from_sdn_value(const Double*, bool);

}}
