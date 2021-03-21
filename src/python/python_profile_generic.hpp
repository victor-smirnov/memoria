
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

#include <memoria/core/container/ctr_referenceable.hpp>

#include "python_store_snapshot_generic.hpp"
#include "python_ctr_referenceable_generic.hpp"
#include "python_btss_iterator_generic.hpp"
#include "python_set_generic.hpp"

namespace memoria {

template <typename Profile>
struct PythonAPIBinder<GenericProfileBindings<Profile>> {

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        PythonAPIBinder<BTSSIterator<Profile>>::make_bindings(m);
        PythonAPIBinder<CtrReferenceable<Profile>>::make_bindings(m);
        PythonAPIBinder<StoreSnapshotOps<Profile>>::make_bindings(m);

        PythonAPIBinder<ICtrApi<Set<Varchar>, Profile>>::make_bindings(m);
        PythonAPIBinder<ICtrApi<Set<UUID>, Profile>>::make_bindings(m);
    }
};


}
