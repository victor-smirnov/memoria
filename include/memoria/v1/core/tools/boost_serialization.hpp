
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

#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/integer/integer.hpp>
#include <memoria/v1/core/strings/string.hpp>

#include <boost/serialization/split_free.hpp>

namespace boost {
namespace serialization {

template<typename Archive, size_t BitLength>
void save(Archive& ar, const memoria::v1::UnsignedAccumulator<BitLength>& acc, const unsigned int version)
{
    ar & acc.to_bmp().str();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::v1::UnsignedAccumulator<BitLength>& acc, const unsigned int version)
{
    using UAcc = memoria::v1::UnsignedAccumulator<BitLength>;

    std::string str;
    ar & str;

    acc = UAcc{str};
}

template<class Archive, size_t BitLength>
inline void serialize(Archive & ar, memoria::v1::UnsignedAccumulator<BitLength>& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::v1::UUID& uuid, const unsigned int version)
{
    ar & uuid.to_u8().to_std_string();
}

template<typename Archive>
void load(Archive& ar, memoria::v1::UUID& uuid, const unsigned int version)
{
    std::string str;
    ar & str;
    uuid = memoria::v1::UUID::parse(str.c_str());
}

template<class Archive>
inline void serialize(Archive & ar, memoria::v1::UUID& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::v1::U8String& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::v1::U8String& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::v1::U8String& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::v1::U16String& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::v1::U16String& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::v1::U16String& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::v1::U32String& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::v1::U32String& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::v1::U32String& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


template<typename Archive>
void save(Archive& ar, const memoria::v1::UWString& str, const unsigned int version)
{
    ar & str.to_std_string();
}

template<typename Archive, size_t BitLength>
void load(Archive& ar, memoria::v1::UWString& u_str, const unsigned int version)
{
    ar & u_str.to_std_string();
}

template<class Archive>
inline void serialize(Archive & ar, memoria::v1::UWString& t, const unsigned int file_version){
    split_free(ar, t, file_version);
}


}
}
