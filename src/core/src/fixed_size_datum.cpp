
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

#include <memoria/v1/api/datatypes/datum.hpp>

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
Datum<DataType> datum_from_sdn_value(const DataType*, const TypedStringValue& value)
{
    U8StringView str = value.text().to_std_string();
    U8StringView tstr = trim_string(str);

    std::istringstream iss (tstr.to_string());

    using Value = typename DataTypeTraits<DataType>::ValueType;

    Value ivalue{};
    iss >> ivalue;

    if (!iss.eof()) {
        MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't convert [{}] to {}", tstr, make_datatype_signature_string<BigInt>());
    }

    return Datum<DataType>(ivalue);
}


template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, const TypedStringValue&);
template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, int64_t);
template Datum<TinyInt>     datum_from_sdn_value(const TinyInt*, double);

template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, const TypedStringValue&);
template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, int64_t);
template Datum<UTinyInt>    datum_from_sdn_value(const UTinyInt*, double);

template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, const TypedStringValue&);
template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, int64_t);
template Datum<SmallInt>    datum_from_sdn_value(const SmallInt*, double);


template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, const TypedStringValue&);
template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, int64_t);
template Datum<USmallInt>   datum_from_sdn_value(const USmallInt*, double);


template Datum<Integer>     datum_from_sdn_value(const Integer*, const TypedStringValue&);
template Datum<Integer>     datum_from_sdn_value(const Integer*, int64_t);
template Datum<Integer>     datum_from_sdn_value(const Integer*, double);

template Datum<UInteger>    datum_from_sdn_value(const UInteger*, const TypedStringValue&);
template Datum<UInteger>    datum_from_sdn_value(const UInteger*, int64_t);
template Datum<UInteger>    datum_from_sdn_value(const UInteger*, double);

template Datum<BigInt>      datum_from_sdn_value(const BigInt*, const TypedStringValue&);
template Datum<BigInt>      datum_from_sdn_value(const BigInt*, int64_t);
template Datum<BigInt>      datum_from_sdn_value(const BigInt*, double);


template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, const TypedStringValue&);
template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, int64_t);
template Datum<UBigInt>     datum_from_sdn_value(const UBigInt*, double);

template Datum<Real>        datum_from_sdn_value(const Real*, const TypedStringValue&);
template Datum<Real>        datum_from_sdn_value(const Real*, int64_t);
template Datum<Real>        datum_from_sdn_value(const Real*, double);


template Datum<Double>      datum_from_sdn_value(const Double*, const TypedStringValue&);
template Datum<Double>      datum_from_sdn_value(const Double*, int64_t);
template Datum<Double>      datum_from_sdn_value(const Double*, double);


}}
