
// Copyright 2016 Victor Smirnov
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



#include <memoria/v1/core/tools/uuid.hpp>
#include <memoria/v1/core/types/type2type.hpp>

#ifndef MMA1_NO_REACTOR
#   include <memoria/v1/reactor/reactor.hpp>
#endif

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <string.h>
#include <chrono>
#include <memory>
#include <sstream>

namespace memoria {
namespace v1 {

namespace {

uint64_t current_time()
{
    auto time = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
}

using BasicRNG = boost::uuids::basic_random_generator<boost::mt19937_64>;

class RNG {
    boost::mt19937_64 mt64_;
    BasicRNG rng_;
public:
    RNG(): rng_(&mt64_)
    {
        // Adding a little bit more randomness
        auto value = std::make_unique<uint64_t>();
        uint64_t ivalue = T2T<uint64_t>(value.get());

        mt64_.seed(current_time() ^ ivalue);
    }

    auto gen() {
        return rng_();
    }
};

RNG& get_RNG() {
    static thread_local RNG rng;
    return rng;
}

}

using UUIDData = decltype(boost::uuids::uuid::data);

UUID make_uuid(const UUIDData& uuid)
{
    UUID uuid2{};

    for (int c = 0; c < 8; c++)
    {
        uuid2.hi() |= ((uint64_t)uuid[c]) << (c * 8);
    }

    for (int c = 0; c < 8; c++)
    {
        uuid2.lo() |= ((uint64_t)uuid[c + 8]) << (c * 8);
    }

    return uuid2;
}


UUID UUID::make_random()
{
    auto uuid = get_RNG().gen();
    return make_uuid(uuid.data);
}

UUID UUID::make_time()
{
    return make_random();
}


UUID UUID::parse(const char* in)
{

    boost::uuids::string_generator gen;

    boost::uuids::uuid v = gen(in);

    return make_uuid(v.data);
}





std::ostream& operator<<(std::ostream& out, const UUID& uuid)
{
    boost::uuids::uuid uu;

    for (int c = 0; c < 8; c++)
    {
        uu.data[c] = uuid.hi() >> (c * 8);
    }

    for (int c = 0; c < 8; c++)
    {
        uu.data[c + 8] = uuid.lo() >> (c * 8);
    }

    out << uu;

    return out;
}

std::istream& operator>>(std::istream& in, UUID& uuid)
{
    char in_buffer[37];
    memset(in_buffer, 0, sizeof(in_buffer));

    std::istream::sentry s(in);

    if (s)
    {
        in.read(in_buffer, sizeof(in_buffer)-1);
        uuid = memoria::v1::UUID::parse(in_buffer);
    }

    return in;
}


U8String UUID::to_u8() const
{
    std::stringstream ss;
    ss << *this;
    return U8String(ss.str());
}

U16String UUID::to_u16() const
{
    return to_u8().to_u16();
}

}}
