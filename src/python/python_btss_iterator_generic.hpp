
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

#include <memoria/api/common/ctr_api_btss.hpp>

#include "python_commons.hpp"

namespace memoria {

template <typename Profile>
struct PythonAPIBinder<BTSSIterator<Profile>> {
    using Type = BTSSIterator<Profile>;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;
        py::class_<Type, CtrSharedPtr<Type>> (m, "BTSSIterator")
                .def("is_end", &Type::is_end)
                .def("next_leaf", &Type::next_leaf)
                .def("next_entry", &Type::next_entry)
                .def("describe", [](Type& ii) noexcept {
                    return wrap_throwing([&]() -> Result<std::string>{
                        std::stringstream ss;
                        MEMORIA_TRY_VOID(ii.dump(ss));
                        return Result<std::string>(ss.str());
                    });
                });
    }
};


}
