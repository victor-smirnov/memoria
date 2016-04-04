
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

#include <uuid/uuid.h>
#include <string.h>

namespace memoria {
namespace v1 {

UBigInt cnt = 1;

UUID make_uuid(uuid_t uuid)
{
    UUID uuid2;

    for (int c = 0; c < 8; c++)
    {
        uuid2.hi() |= ((UBigInt)uuid[c]) << (c * 8);
    }

    for (int c = 0; c < 8; c++)
    {
        uuid2.lo() |= ((UBigInt)uuid[c + 8]) << (c * 8);
    }

    return uuid2;
}

UUID UUID::make_random()
{
    uuid_t uuid;

    uuid_generate_random(uuid);

    return make_uuid(uuid);
//	return UUID(0, cnt++);
}

UUID UUID::make_time()
{
    uuid_t uuid;

    uuid_generate_time_safe(uuid);

    return make_uuid(uuid);
}


UUID UUID::parse(const char* in)
{
    uuid_t uu;

    uuid_parse(in, uu);

    return make_uuid(uu);
}

std::ostream& operator<<(std::ostream& out, const UUID& uuid)
{
    uuid_t uu;

    for (int c = 0; c < 8; c++)
    {
        uu[c] = uuid.hi() >> (c * 8);
    }

    for (int c = 0; c < 8; c++)
    {
        uu[c + 8] = uuid.lo() >> (c * 8);
    }

    char out_buffer[37];

    uuid_unparse(uu, out_buffer);

    out<<out_buffer;

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
        uuid = UUID::parse(in_buffer);
    }

    return in;
}

}}
