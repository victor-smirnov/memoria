
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

#include <memoria/python/python_commons.hpp>

namespace pybind11 {
template<typename Profile>
struct polymorphic_type_hook<memoria::CtrReferenceable<Profile>> {
    using Type = memoria::CtrReferenceable<Profile>;

    static const void *get(const Type *src, const std::type_info*& type) {
            // note that src may be nullptr
            if (src) {
                type = &src->api_type_info();
            }
            return src;
        }
};
} // namespace pybind11

namespace memoria {

template <typename Profile>
struct PythonAPIBinder<CtrReferenceable<Profile>> {

    using CtrReferenceableType = CtrReferenceable<Profile>;
    using CtrID = ApiProfileCtrID<Profile>;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        py::class_<CtrReferenceableType, CtrSharedPtr<CtrReferenceableType>> ctr_referenceable(m, "CtrReferenceable");
        ctr_referenceable.def("is_castable_to", &CtrReferenceableType::is_castable_to);
        ctr_referenceable.def("describe_type", &CtrReferenceableType::describe_type);
        ctr_referenceable.def("describe_datatype", &CtrReferenceableType::describe_datatype);
        ctr_referenceable.def("type_hash", &CtrReferenceableType::type_hash);
        ctr_referenceable.def("set_new_block_size", &CtrReferenceableType::set_new_block_size);
        ctr_referenceable.def("get_new_block_size", &CtrReferenceableType::get_new_block_size);
        ctr_referenceable.def("get_ctr_property", &CtrReferenceableType::get_ctr_property);
        ctr_referenceable.def("set_ctr_property", &CtrReferenceableType::set_ctr_property);
        ctr_referenceable.def("remove_ctr_property", &CtrReferenceableType::remove_ctr_property);
        ctr_referenceable.def("ctr_properties", &CtrReferenceableType::ctr_properties);

        //for_each_ctr_property
        ctr_referenceable.def("for_each_ctr_property", &CtrReferenceableType::for_each_ctr_property);
        ctr_referenceable.def("set_ctr_properties", &CtrReferenceableType::set_ctr_properties);
        ctr_referenceable.def("get_ctr_reference", &CtrReferenceableType::get_ctr_reference);
        ctr_referenceable.def("set_ctr_reference", &CtrReferenceableType::set_ctr_reference);
        ctr_referenceable.def("remove_ctr_reference", &CtrReferenceableType::remove_ctr_reference);
        ctr_referenceable.def("ctr_references", &CtrReferenceableType::ctr_references);

        //for_each_ctr_reference
        ctr_referenceable.def("for_each_ctr_reference", [](CtrReferenceableType& ctr, std::function<void (U8StringView, const CtrID&)> consumer){
            return ctr.for_each_ctr_reference([&](U8StringView name, const CtrID& ctr_id) {
                consumer(name, ctr_id);
            });
        });
        ctr_referenceable.def("set_ctr_references", &CtrReferenceableType::set_ctr_references);

        ctr_referenceable.def("name", &CtrReferenceableType::name);
        ctr_referenceable.def("drop", &CtrReferenceableType::drop);
        ctr_referenceable.def("flush", &CtrReferenceableType::flush);

    }
};


}
