
// Copyright 2019 Victor Smirnov
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
#include <memoria/core/strings/u8_string.hpp>

namespace memoria {

class SDNEntryView;
class SDNArrayView;
class SDNTypedeclView;
class SDNMapView;
class SDNDatum;

class SDNArrayView {
public:
    size_t size() const;
    SDNDatum operator[](size_t idx) const;

    template <typename T>
    T get_as(size_t idx) const;
};

class SDNMapView {
public:
    size_t size() const;
    SDNDatum get(U8StringView name) const;

    template <typename T>
    T get_as(size_t idx) const;
};

}
