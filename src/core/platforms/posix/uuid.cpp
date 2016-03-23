
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/v1/core/tools/uuid.hpp>

#include <uuid/uuid.h>
#include <string.h>

namespace memoria {

UBigInt cnt = 1;

UUID make_uuid(uuid_t uuid)
{
    UUID uuid2;

    for (int c = 0; c < 8; c++)
    {
        uuid2.lo() |= ((UBigInt)uuid[c]) << (c * 8);
    }

    for (int c = 0; c < 8; c++)
    {
        uuid2.hi() |= ((UBigInt)uuid[c + 8]) << (c * 8);
    }

    return uuid2;
}

UUID UUID::make_random()
{
    uuid_t uuid;

    uuid_generate_random(uuid);

    return make_uuid(uuid);
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
        uu[c] = uuid.lo() >> (c * 8);
    }

    for (int c = 0; c < 8; c++)
    {
        uu[c + 8] = uuid.hi() >> (c * 8);
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

}

