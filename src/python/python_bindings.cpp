
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


#include <memoria/api/store/swmr_store_api.hpp>
#include <memoria/profiles/memory_cow/memory_cow_profile.hpp>
#include <memoria/memoria.hpp>

#include "python_global_bindings.hpp"
#include "python_cow_profile_global_bindings.hpp"

#include "python_result_t.hpp"
#include "python_store_api_swmr.hpp"

namespace py = pybind11;
using namespace memoria;

struct Base {
    virtual ~Base() noexcept = default;
    virtual void say_hello() noexcept = 0;
};

struct Derived: Base {
    void say_hello() noexcept {
        std::cout << "HELLO!" << std::endl;
    }
};

Result<std::shared_ptr<Base>> make_derived() {
    return Result<std::shared_ptr<Base>>::of(new Derived{});
}

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

    PythonAPIBinder<GlobalBindings>::make_bindings(m);
    py::module_ m_cow = m.def_submodule("cow");

    PythonAPIBinder<MemoryCoWProfile<>>::make_bindings(m_cow);
    PythonAPIBinder<ISWMRStore<MemoryCoWProfile<>>>::make_bindings(m_cow);

    py::class_<Base, std::shared_ptr<Base>>(m, "Base");
    py::class_<Derived, Base, std::shared_ptr<Derived>>(m, "Derived")
            .def("say_hello", &Derived::say_hello)
            ;

    m.def("make_derived", &make_derived);
}
