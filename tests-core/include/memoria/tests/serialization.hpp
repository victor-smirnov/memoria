
// Copyright 2018 Victor Smirnov
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

#include <memoria/core/tools/boost_serialization.hpp>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/unordered_map.hpp>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

#include <boost/filesystem.hpp>

#include <fstream>

namespace memoria {
namespace tests {

namespace fs = boost::filesystem;

struct ConfigurationContext {
    virtual ~ConfigurationContext() noexcept {}
    virtual fs::path resource_path(const std::string& name) = 0;
};


template <typename T>
struct IndirectStateFiledSerializer {

    static void externalize(const T& field, fs::path path, ConfigurationContext* context)
    {
        std::fstream fos;
        fos.open(path.string(), fos.out | fos.trunc);

        boost::archive::text_oarchive ar(fos);

        ar << field;

        fos.flush();
        fos.close();
    }

    static void internalize(T& field, fs::path path, ConfigurationContext* context)
    {
        std::fstream fis;
        fis.open(path.string(), fis.in);

        boost::archive::text_iarchive ar(fis);

        ar >> field;

        fis.close();
    }
};


}}
