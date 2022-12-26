
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

#include <memoria/core/hermes/hermes.hpp>
#include <memoria/core/tools/result.hpp>

namespace memoria {

enum class CheckSeverity {
    ERROR
};

using CheckResultConsumerFn = std::function<void (CheckSeverity, const hermes::HermesCtrPtr&)>;

struct NullCheckResultConsumer {
    void operator()(CheckSeverity, const hermes::HermesCtrPtr&) {}
};

struct ThrowingCheckResultConsumer {
    void operator()(CheckSeverity svr, const hermes::HermesCtrPtr& doc) {
        if (svr == CheckSeverity::ERROR) {
            MEMORIA_MAKE_GENERIC_ERROR("{}", doc->to_pretty_string()).do_throw();
        }
    }
};

template <typename... Args>
hermes::HermesCtrPtr make_string_document(const char* fmt, Args&&... args)
{
    auto str = format_u8(fmt, std::forward<Args>(args)...);
    hermes::HermesCtrPtr doc = hermes::HermesCtr::make_pooled();
    auto vv = doc->make_t<Varchar>(str);
    doc->set_root(vv);
    return doc;
}

}
