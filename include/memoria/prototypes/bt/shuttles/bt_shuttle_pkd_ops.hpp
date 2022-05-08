
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

#include <memoria/prototypes/bt/shuttles/bt_shuttle_base.hpp>

namespace memoria::bt {

class PkdFindSumFwFn {
    SearchType search_type_;

public:
    PkdFindSumFwFn(SearchType search_type):
        search_type_(search_type)
    {}


    template <int32_t StreamIdx, typename Tree, typename KeyType, typename AccumType>
    ShuttleOpResult stream(const Tree& tree, size_t column, size_t start, const KeyType& key, AccumType& sum)
    {
        auto size = tree.size();

        if (start < size)
        {
            KeyType k = key - sum;

            auto result = tree.findForward(search_type_, column, start, k);

            sum += result.prefix();

            return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() < size);
        }
        else {
            return ShuttleOpResult::empty();
        }
    }
};

class PkdFindSumBwFn {
    SearchType search_type_;

public:
    PkdFindSumBwFn(SearchType search_type):
        search_type_(search_type)
    {}


    template <int32_t StreamIdx, typename Tree, typename KeyType, typename AccumType>
    ShuttleOpResult stream(const Tree& tree, size_t column, size_t start, const KeyType& key, AccumType& sum)
    {
        auto size = tree.size();

        if (start == std::numeric_limits<size_t>::max()) {
            return ShuttleOpResult::empty();
        }
        else {
            if (start >= size) {
                start = size - 1;
            }

            if (search_type_ == SearchType::GT)
            {
                auto result = tree.findGTBackward(column, start, key);
                sum += result.prefix();
                return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() <= start);
            }
            else {
                auto result = tree.findGEBackward(column, start, key);
                sum += result.prefix();
                return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() <= start);
            }
        }
    }
};


class PkdFindMaxFwFn {
    SearchType search_type_;
public:
    PkdFindMaxFwFn(SearchType search_type):
        search_type_(search_type)
    {}

    template <int32_t StreamIdx, typename Tree, typename KeyType>
    ShuttleOpResult stream(const Tree& tree, size_t column, const KeyType& key)
    {
        auto size = tree.size();
        if (size) {
          if (search_type_ == SearchType::GT)
          {
            auto result = tree.find_fw_gt(column, key);
            return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() < size);
          }
          else {
            auto result = tree.find_fw_ge(column, key);
            return ShuttleOpResult::non_empty(result.local_pos(), result.local_pos() < size);
          }
        }
        else {
          return ShuttleOpResult::empty();
        }
    }
};




//template <typename KeyType, typename AccumType = KeyType>
//class PkdSelectFwFn {
//    SearchType search_type_;
//public:
//    PkdSelectFwFn(SearchType search_type):
//        search_type_(search_type)
//    {}


//    template <int32_t StreamIdx, typename Tree>
//    StreamOpResult stream(const Tree& tree, size_t column, size_t start)
//    {
//        auto size = tree.size();

//        if (start < size)
//        {
//            KeyType k = target_ - sum_;

//            auto result = tree.findForward(search_type_, column, start, k);

//            sum_ += result.prefix();

//            return StreamOpResult(result.local_pos(), start, result.local_pos() >= size, false);
//        }
//        else {
//            return StreamOpResult(size, start, true, true);
//        }
//    }
//};






}
