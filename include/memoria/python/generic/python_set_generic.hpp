
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

#include <memoria/api/set/set_api.hpp>
#include <memoria/api/set/set_api_factory.hpp>

#include <memoria/core/datatypes/datatypes.hpp>

#include <memoria/python/python_commons.hpp>

namespace memoria {

template <typename Key, typename Profile>
struct PythonAPIBinder<ICtrApi<Set<Key>, Profile>> {

    using CtrType               = ICtrApi<Set<Key>, Profile>;
    using CtrReferencableType   = CtrReferenceable<Profile>;
    using BTSSIterType          = BTSSIterator<Profile>;
    using IterType              = SetIterator<Key, Profile>;
    using KeyView               = DTTViewType<Key>;
    using CtrSizeT              = typename CtrType::CtrSizeT;
    using Buffer                = DataTypeBuffer<Key>;

    using ProducerFn            = typename CtrType::ProducerFn;

    using Producer2Fn        = std::function<bool (Buffer*, size_t)>;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;

        py::class_<IterType, BTSSIterType, CtrSharedPtr<IterType>>(m, get_datatype_script_name<Key>("SetIterator").c_str())
                .def("is_end", &IterType::is_end)
                .def("next", &IterType::next)
                .def("key", &IterType::key)
                ;

        py::class_<CtrType, CtrReferencableType, CtrSharedPtr<CtrType>> set_cls(m, get_datatype_script_name<Key>("Set").c_str());
                set_cls.def("find", &CtrType::find);
                set_cls.def("size", &CtrType::size);
                set_cls.def("contains", &CtrType::contains);
                set_cls.def("remove", py::overload_cast<KeyView>(&CtrType::remove));
                set_cls.def("insert", py::overload_cast<KeyView>(&CtrType::insert));
                set_cls.def("insert_buf", py::overload_cast<CtrSizeT, const Buffer&>(&CtrType::insert));
                //set_cls.def("append", py::overload_cast<ProducerFn>(&CtrType::append));
                set_cls.def("append", [](CtrType& ctr, Producer2Fn fn) {
                    return ctr.append([&](Buffer& buf, size_t size){
                        return fn(&buf, size);
                    });
                });
                set_cls.def("for_each", [](CtrType& ctr, std::function<void (KeyView)> fn) {
                    return ctr.for_each([&](auto key){
                        return fn(key);
                    });
                });

        set_cls.doc() = format_u8(R"(
            Set<{}>
        )", make_datatype_signature<Key>().name());

    }
};


}
