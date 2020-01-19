
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

#include <memoria/core/tools/uuid.hpp>
#include <memoria/core/tools/optional.hpp>
#include <memoria/core/integer/integer.hpp>
#include <memoria/core/strings/string.hpp>

#include <boost/serialization/split_free.hpp>

namespace memoria {
namespace serialization {

template<typename Archive, size_t BitLength>
void save(Archive& ar, const memoria::UnsignedAccumulator<BitLength>& acc, const unsigned int version)
{
    ar & acc.to_bmp().str();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::UnsignedAccumulator<BitLength>& acc, const unsigned int version)
{
    using UAcc = memoria::UnsignedAccumulator<BitLength>;

    std::string str;
    ar & str;

    acc = UAcc{str};
}

template<class Archive, size_t BitLength>
inline void serialize(Archive & ar, memoria::UnsignedAccumulator<BitLength>& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::UUID& uuid, const unsigned int version)
{
    ar & uuid.to_u8().to_std_string();
}

template<typename Archive>
void load(Archive& ar, memoria::UUID& uuid, const unsigned int version)
{
    std::string str;
    ar & str;
    uuid = memoria::UUID::parse(str.c_str());
}

template<class Archive>
inline void serialize(Archive & ar, memoria::UUID& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::U8String& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::U8String& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::U8String& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::U16String& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::U16String& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::U16String& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::U32String& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::U32String& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::U32String& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::UWString& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::UWString& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::UWString& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}




template<typename Archive, typename T>
void save(Archive& ar, const memoria::Optional<T>& opt, const unsigned int version)
{
    bool non_empty = (bool)opt;
    ar & non_empty;

    if (non_empty)
    {
        ar & opt.value();
    }
}

template<typename Archive, typename T>
void load(Archive& ar, memoria::Optional<T>& opt, const unsigned int version)
{
    bool non_empty{};
    ar & non_empty;
    if (non_empty)
    {
        T value{};
        ar & value;
        opt = memoria::Optional<T>(value);
    }
    else {
        opt = memoria::Optional<T>();
    }
}

template<typename Archive, typename T>
inline void serialize(Archive & ar, memoria::Optional<T>& opt, const unsigned int file_version){
    split_free(ar, opt, file_version);
}


}
}

