
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

#include <memoria/python/python_commons.hpp>
#include <memoria/python/python_datum_bindings.hpp>
#include "python_datum_bindings.hpp"

namespace memoria {

static inline void define_UUID(pybind11::module_& m) {
    namespace py = pybind11;
    py::class_<UUID>(m, "UUID")
            .def(py::init())
            .def(py::init<uint64_t, uint64_t>(), py::arg("hi"), py::arg("lo"), "Create UUID using high and low unsigned 64 bit values of the UUID")
            .def("hi", [](const UUID& uuid){
                return uuid.hi();
            }, "Return high parth of the UUID as unsigned 64 bit value")
            .def("lo", [](const UUID& uuid){
                return uuid.lo();
            }, "Return low parth of the UUID as unsigned 64 bit value")
            .def("is_null", &UUID::is_null, "Check if both high and low parts are 0")
            .def("is_set", &UUID::is_set, "Inverse of is_null()")
            .def("clear", &UUID::clear, "Write 0 into both high and low parts")
            .def("equals", [](const UUID& uuid1, const UUID& uuid2){
                return uuid1 == uuid2;
            }, "Compare two UUIDs for data equiality")
            .def("__eq__", &UUID::operator==)
            .def("to_string", &UUID::to_u8)
            .def("__repr__", &UUID::to_u8)
            .def("__hash__", [](UUID& uuid){
                return std::hash<UUID>{}(uuid);
            })
            .def_static("make_random", &UUID::make_random, "Create a random UUID")
            .def_static("make_time", &UUID::make_time, "Currently create a random UUID, not a Time UUID!")
            .def_static("parse", &UUID::parse, "Create UUID by parsing it's standard string representation");
}


static inline void define_LifetimeGuard(pybind11::module_& m, U8String type_name) {
    namespace py = pybind11;
    using Type = LifetimeGuard;

    py::class_<Type>(m, type_name.data())
            .def(py::init())
            .def("is_valid", &Type::is_valid)
            .def("__repr__", [](const Type& lg) -> std::string {
                return lg.is_valid() ? "LifetimeGuard[valid]" : "LifetimeGuard[invalid]";
            })
            ;
}



template <>
struct PythonAPIBinder<GlobalBindings> {
    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;
        define_UUID(m);
        define_pair<UUID, UUID>(m, "UUIDPair");
        define_pair<U8String, U8String>(m, "StringPair");
        define_pair<U8String, UUID>(m, "StringUUIDPair");

        define_LifetimeGuard(m, "LifetimeGuard");
    }
};




}
