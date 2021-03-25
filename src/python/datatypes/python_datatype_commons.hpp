
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

#include <memoria/core/datatypes/datatypes.hpp>
#include <memoria/core/datatypes/buffer/buffer_generic.hpp>

#include "../python_commons.hpp"

namespace memoria {

template <typename DataType, typename ViewType>
struct PythonAPIDatatypeBinderBase {
    using ViewT = GuardedView<ViewType>;

    static pybind11::class_<ViewT> make_bindings_base(pybind11::module_& m, const char* class_name) {
        pybind11::class_<ViewT> cls(m, class_name);
        return cls;
    }
};

template <typename Builder>
class BuilderGuard {
    Builder* builder_;
    LifetimeGuard guard_;
public:
    BuilderGuard(Builder* builder, LifetimeGuard guard) noexcept :
        builder_(builder), guard_(guard)
    {}

    LifetimeGuard builder_guard() noexcept {
        return guard_;
    }

    Builder& builder() {
        if (guard_.is_valid()) {
            return *builder_;
        }
        throw std::runtime_error("Accessing stale DataTypeBuffer");
    }
};



template <typename DataType>
struct PythonAPIDatatypeBufferBase {
    using Type = DataTypeBuffer<DataType>;
    using Builder = typename Type::Builder;

    static pybind11::class_<Type> make_bindings_base(pybind11::module_& m, const char* class_name) {
        namespace py = pybind11;
        py::class_<Type> cls(m, class_name);

        cls.def(py::init());
        cls.def("guard", &Type::data_guard);
        //cls.def("builder", &Type::builder);
        cls.def("size", &Type::size);
        cls.def("get", [](Type& buf, size_t idx){
            if (idx < buf.size()) {
                return buf.get_guarded(idx);
            }
            else {
                throw std::runtime_error("Range check failure in DataBuffer");
            }
        });
        cls.def("clear", &Type::clear);
        cls.def("reset", &Type::reset);
        cls.def("describe", &Type::describe);

        cls.def("__repr__", &Type::describe);

        cls.def("builder", [](Type& buffer) {
            return BuilderGuard<Builder>(&buffer.builder(), buffer.buffer_guard());
        });

        return cls;
    }
};


template <typename DataType, typename Buffer = DataTypeBuffer<DataType>>
struct PythonAPIDatatypeBufferBuilderBase {
    using Builder = SparseObjectBuilder<DataType, Buffer>;
    using Type = BuilderGuard<Builder>;

    static pybind11::class_<Type> make_bindings_base(pybind11::module_& m, const char* class_name) {
        namespace py = pybind11;
        py::class_<Type> cls(m, class_name);

        cls.def("reset", [](Type& builder){
            return builder.builder().reset();
        });
        cls.def("build", [](Type& builder){
            return builder.builder().build();
        });
        cls.def("view", [](Type& builder){
            return builder.builder().guarded_view();
        });
        cls.def("is_empty", [](Type& builder){
            return builder.builder().is_empty();
        });

        std::string cls_name = class_name;
        cls.def("__repr__", [=](Type& builder){
            return std::string("memoria.") + std::string(cls_name) + "[" + (builder.builder_guard().is_valid() ? "valid]" : "invalid]");
        });

        return cls;
    }
};


}
