
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

#include <memoria/core/strings/format.hpp>
#include <memoria/python/python_commons.hpp>

#include <codegen.hpp>

#include <pybind11/embed.h>

#include <fstream>

namespace py = pybind11;

using namespace clang;

namespace memoria {
namespace codegen {

void create_ast_module(pybind11::module mm) {
    py::class_<QualType> qual_type(mm, "QualType");
    qual_type.def("__str__", [](const QualType& qt){
        return qt.getAsString();
    });
}


}}
