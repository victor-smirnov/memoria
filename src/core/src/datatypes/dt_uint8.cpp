
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

#include <memoria/v1/core/exceptions/exceptions.hpp>

#include <memoria/v1/core/linked/datatypes/default_datatype_ops.hpp>
#include <memoria/v1/core/linked/datatypes/type_registry.hpp>
#include <memoria/v1/core/linked/datatypes/datum.hpp>

#include <string>


namespace memoria {
namespace v1 {

template <>
struct DataTypeOperationsImpl<UTinyInt>: DataTypeOperations {
    virtual boost::any create_cxx_instance(
        DataTypeRegistry& registry,
        const LDTypeDeclaration& typedecl
    ) {
        return boost::any(UTinyInt{});
    }

    virtual AnyDatum from_ld_document(
        const DataTypeRegistry& registry,
        const LDDValue& value
    ){
        if (!value.is_null())
        {
            LDString str = value.as_string();
            U8String u8str(str.view());

            size_t err_idx = 0;
            try {
                int vv = std::stoi(u8str.to_std_string(), &err_idx, 10);
                if (vv >= 0 && vv < 256) {
                    return Datum<UTinyInt>(vv);
                }
                else {
                    MMA1_THROW(BoundsException()) << fmt::format_ex(u"Value is out of bounds [0, 255]", u8str);
                }
            }
            catch (std::invalid_argument& ex) {
                MMA1_THROW(RuntimeException()) << fmt::format_ex(u"Can't convert value {} to UTinyInt", u8str);
            }
            catch (std::out_of_range& ex) {
                MMA1_THROW(BoundsException()) << fmt::format_ex(u"Value is out of bounds [0, 255]", u8str);
            }
        }
        else {
            return AnyDatum();
        }
    }
};

//DataTypeRegistryStore::OpsInitializer<UTinyInt> u_tiny_int_;

}}
