
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

#include "python_commons.hpp"



namespace memoria {

class DataTypes {};

template <typename T>
struct PythonAPIBinder<Datum<T>> {

    using Type = Datum<T>;

    static void make_bindings(pybind11::module_& m, std::string datatype_name = "") {
        namespace py = pybind11;

        std::string type_name;
        if (datatype_name == "") {
            type_name = get_datatype_script_name<T>("");
        }
        else {
            type_name = datatype_name;
        }

        py::class_<Type>(m, type_name.c_str())
                .def("view", &Type::view)
                .def("__repr__", &Type::to_sdn_string)
                .def("__bool__", &Type::operator bool)
                .def_static("from_sdn", &Type::from_sdn_string)
                ;
    }
};


template <>
struct PythonAPIBinder<DataTypes> {
    static void make_bindings(pybind11::module_& m)
    {
        PythonAPIBinder<Datum<Varchar>>::make_bindings(m);
        PythonAPIBinder<Datum<BigInt>>::make_bindings(m);
        PythonAPIBinder<Datum<UUID>>::make_bindings(m, "UUIDDatum");
    }
};

}
