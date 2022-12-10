
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

#include <memoria/core/hermes/hermes.hpp>
#include <vector>

namespace memoria::hermes {

class TplVarStack {
public:
    struct Entry {
        U8StringView name;
        Object value;
        bool frame_start;
    };

private:
    std::vector<Entry> stack_;

public:
    TplVarStack() {}

    void start_frame(U8StringView name, const Object& value) {
        stack_.emplace_back<Entry>({name, value, true});
    }

    void set(U8StringView name, const Object& value) {
        stack_.emplace_back<Entry>({name, value, false});
    }

    void pop() {
        do {
            stack_.pop_back();
        }
        while (stack_.size() > 0 && !stack_.back().frame_start);
    }

    Optional<Object> find(U8StringView name) const
    {
        for (auto ii = stack_.rbegin(); ii != stack_.rend(); ii++) {
            if (ii->name == name) {
                return ii->value;
            }
        }

        return {};
    }
};


}
