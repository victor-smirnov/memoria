
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

#include <memoria/memoria.hpp>
#include <memoria/python/python_result_t.hpp>
#include <memoria/python/generic/python_store_api_swmr.hpp>

#include <memoria/profiles/core_api/core_api_profile.hpp>

#include "python_global_bindings.hpp"
#include "python_cow_profile_global_bindings.hpp"


namespace py = pybind11;
using namespace memoria;

std::string memoria::get_script_name(const std::string& in) {
    SBuf buf;
    for (size_t c = 0; c < in.length(); c++) {
        if (in[c] == '<') {
            buf << "_";
        }
        else if (in[c] == '>') {
            buf << "_";
        }
        else {
            buf << in[c];
        }
    }

    return buf.str();
}


PYBIND11_MODULE(memoria, m) {
    InitMemoriaExplicit();

    m.doc() = R"(
        Python bindings for the Memoria library.

        These bindings are mainly intended for adhoc operations
        on Memoria containers to accomodate Memoria development.
    )";

    PythonAPIBinder<GlobalBindings>::make_bindings(m);

    py::module_ m_datatypes = m.def_submodule("datatypes");

    m_datatypes.doc() = R"(
        Data types are special kind of data object that can be stored in
        data containers. Most of these data types mimique API of corresponding
        C++ objects, so the python code using them can be easily ported to
        C++ if performance matters.
    )";

    PythonAPIBinder<DataTypes>::make_bindings(m_datatypes);


    py::module_ m_core_api = m.def_submodule("coreapi");

    m_core_api.doc() = R"(
        Python module for Memoria's CoreApiProfile (storage engines,
        data container, query engines and auxiliary tools).
    )";

    py::module_ m_core_api_ctrs = m_core_api.def_submodule("containers");

    PythonAPIBinder<CoreApiProfile<>>::make_bindings(m_core_api_ctrs);

    py::module_ m_core_api_stores = m_core_api.def_submodule("stores");
    PythonAPIBinder<ISWMRStore<CoreApiProfile<>>>::make_bindings(m_core_api_stores);
}
