
// Copyright 2023 Victor Smirnov
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

#include <memoria/core/flat_map/flat_hash_map.hpp>

#include <vector>
#include <unordered_set>


namespace memoria {

class BlockIDSet {
    ska::flat_hash_set<uint64_t> set_;
public:

    void add(uint64_t value) {
        set_.emplace(value);
    }

    void remove(uint64_t value) {
        set_.erase(value);
    }

    bool contains(uint64_t value) const {
        return set_.find(value) != set_.end();
    }

    void clear() {
        set_.clear();
    }
};

}
