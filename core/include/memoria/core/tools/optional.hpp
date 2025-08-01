
// Copyright 2015 Victor Smirnov
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

#include <memoria/core/types.hpp>
#include <fmt/format.h>

#include <optional>
#include <vector>
#include <ostream>


namespace memoria {


template <typename T>
using Optional = std::optional<T>;

template <typename T>
struct HasFieldFactory<Optional<T>> : HasFieldFactory<T> {};

template <typename T>
struct HasValueCodec<Optional<T>> : HasValueCodec<T> {};

template <typename T>
std::ostream& operator<<(std::ostream& out, const Optional<T>& val) {
    if (val) {
        out << val.value();
    }
    else {
        out << "{Empty}";
    }
    return out;
}

}

namespace fmt {

template <typename T>
struct formatter<memoria::Optional<T>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::Optional<T>& d, FormatContext& ctx) const {
        if (d.is_initialized()) {
            return formatter<T>().format(d.get(), ctx);
        }
        else {
            return fmt::format_to(ctx.out(), "{}", "[EmptyOptional]");
        }
    }
};

}
