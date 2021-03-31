
// Copyright 2021 Victor Smirnov
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

#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/strings/string.hpp>
#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/python/python_commons.hpp>
#include <memoria/python/python_datum_bindings.hpp>

#include "datatypes/python_varchar_datatype.hpp"

#include <string>
#include <sstream>

namespace memoria {

template <>
struct PythonAPIBinder<DataTypes> {
    static void make_bindings(pybind11::module_& m)
    {
        PythonAPIBinder<Datum<Varchar>>::make_bindings(m);
        PythonAPIBinder<Datum<BigInt>>::make_bindings(m);
        PythonAPIBinder<Datum<UUID>>::make_bindings(m, "UUIDDatum");

        PythonAPIBinder<GuardedView<U8StringView>>::make_bindings(m);

        PythonAPIBinder<Varchar>::make_bindings(m);
    }
};

}
