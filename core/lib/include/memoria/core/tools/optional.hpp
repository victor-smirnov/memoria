
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

#include <vector>
#include <ostream>

#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

#include <fmt/format.h>

namespace memoria {

template <typename T>
using Optional = boost::optional<T>;


template <typename T>
struct HasFieldFactory<Optional<T>> : HasFieldFactory<T> {};

template <typename T>
struct HasValueCodec<Optional<T>> : HasValueCodec<T> {};

}

namespace fmt {

template <typename T>
struct formatter<memoria::Optional<T>> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const memoria::Optional<T>& d, FormatContext& ctx) {
        if (d.is_initialized()) {
            return formatter<T>().format(d.get(), ctx);
        }
        else {
            return format_to(ctx.out(), "{}", "[EmptyOptional]");
        }
    }
};


}
