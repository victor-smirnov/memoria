
// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <memoria/v1/core/types/types.hpp>

#include <vector>
#include <ostream>

namespace memoria {

template <typename T>
class Optional {
    T value_;
    bool is_set_;
public:
    template <typename TT>
    Optional(TT&& value, bool is_set = true): value_(value), is_set_(is_set) {}
    Optional(): is_set_(false) {}

    const T& value() const {
        return value_;
    }

    bool is_set() const {
        return is_set_;
    }

    operator bool() const {
        return is_set_;
    }

    const T* operator->() const {
        return &value_;
    }

    const T& operator*() const {
        return value_;
    }

    //TODO: Other methods for this monade
};

template <typename T>
std::ostream& operator<<(std::ostream& out, const Optional<T>& op) {
    if (op) {
        out<<op.value();
    }
    else {
        out<<"[none]";
    }
    return out;
}


}
