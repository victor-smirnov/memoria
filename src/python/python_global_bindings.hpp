
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

#include "python_commons.hpp"
#include "python_datum_bindings.hpp"

namespace memoria {

static inline void define_UUID(pybind11::module_& m) {
    namespace py = pybind11;
    py::class_<UUID>(m, "UUID")
            .def(py::init())
            .def(py::init<uint64_t, uint64_t>())
            .def("hi", [](const UUID& uuid){
                return uuid.hi();
            })
            .def("lo", [](const UUID& uuid){
                return uuid.lo();
            })
            .def("is_null", &UUID::is_null)
            .def("is_set", &UUID::is_set)
            .def("clear", &UUID::clear)
            .def("equals", [](const UUID& uuid1, const UUID& uuid2){
                return uuid1 == uuid2;
            })
            .def("__eq__", &UUID::operator==)
            .def("to_string", &UUID::to_u8)
            .def("__repr__", &UUID::to_u8)
            .def_static("make_random", &UUID::make_random)
            .def_static("make_time", &UUID::make_time)
            .def_static("parse", &UUID::parse);
}

template <typename T1, typename T2>
static inline void define_pair(pybind11::module_& m, U8String type_name) {
    namespace py = pybind11;
    using Type = std::pair<T1, T2>;

    py::class_<Type>(m, type_name.data())
            .def(py::init())
            .def(py::init<T1, T2>())
            .def("first", [](const Type& pair){
                return pair.first;
            })
            .def("second", [](const Type& pair){
                return pair.second;
            });
}



template <>
struct PythonAPIBinder<GlobalBindings> {
    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;
        define_UUID(m);
        define_pair<UUID, UUID>(m, "UUIDPair");
        define_pair<U8String, U8String>(m, "StringPair");
        define_pair<U8String, UUID>(m, "StringUUIDPair") ;

        PythonAPIBinder<DataTypes>::make_bindings(m);
    }
};




}
