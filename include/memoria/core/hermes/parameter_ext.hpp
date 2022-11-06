
// Copyright 2022 Victor Smirnov
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

#include <memoria/core/hermes/parameter.hpp>
#include <memoria/core/hermes/container.hpp>

namespace memoria {
namespace hermes {

template <typename DT>
inline void Params::add_dataobject(U8StringView name, DTTViewType<DT> view) {
    auto doc = HermesCtr::make_pooled();
    params_[name] = doc->set_dataobject<DT>(view)->as_value();
}


inline void Params::add_hermes(U8StringView name, U8StringView value) {
    auto doc = HermesCtr::parse_document(value);
    params_[name] = doc->root();
}

inline bool Params::has_parameter(U8StringView name) const {
    return params_.find(name) != params_.end();
}

inline ValuePtr Params::resolve(U8StringView name) const {
    auto ii = params_.find(name);
    if (ii != params_.end()) {
        return ii->second;
    }

    return ValuePtr{};
}



}}
