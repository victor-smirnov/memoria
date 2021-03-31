
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

#include <memoria/python/datatypes/python_datatype_commons.hpp>

namespace memoria {

template <>
struct PythonAPIBinder<Varchar> {
    using DataType = Varchar;
    using ViewType = DTTViewType<DataType>;

    using BufferType = DataTypeBuffer<DataType>;
    using BuilderType = BuilderGuard<typename DataTypeBuffer<DataType>::Builder>;

    using KeysSubstream     = DataTypeBuffer<Varchar>;
    using ProducerFn        = std::function<bool (KeysSubstream*, size_t)>;

    static void make_bindings(pybind11::module_& m) {
        namespace py = pybind11;
        //auto view_cls = PythonAPIDatatypeBinderBase<DataType, ViewType>::make_bindings_base(m, "VarcharView");

        auto buffer_builder_cls = PythonAPIDatatypeBufferBuilderBase<DataType>::make_bindings_base(m, "VarcharBufferBuilder");

        buffer_builder_cls.def("append", [](BuilderType& builder, std::string arg){
            return builder.builder().append(arg);
        });

        auto buffer_cls = PythonAPIDatatypeBufferBase<DataType>::make_bindings_base(m, "VarcharBuffer");

        buffer_cls.def("append", [](BufferType& buf, std::string arg){
            buf.append(arg);
        });

        buffer_cls.def("sort", &BufferType::sort);
    }
};

}
