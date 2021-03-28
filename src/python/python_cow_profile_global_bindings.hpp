
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

#include "python_profile_generic.hpp"

namespace memoria {

template <typename T>
struct PythonAPIBinder<CoreCowApiProfile<T>> {
    using Profile = CoreCowApiProfile<T>;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        PythonAPIBinder<GenericProfileBindings<Profile>>::make_bindings(m);
    }
};


}
