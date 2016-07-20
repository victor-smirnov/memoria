
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

//#include <uuid/uuid.h>
#include <Rpc.h>

#include <string.h>

#include <mutex>

namespace memoria {
namespace v1 {

using WinUUID = ::UUID;

UBigInt cnt = 1;

UUID make_uuid(const WinUUID& uuid)
{
    UUID uuid2;

    uuid2.lo() = uuid.Data1;

    uuid2.lo() |= ((UBigInt)uuid.Data2) << 32;
    uuid2.lo() |= ((UBigInt)uuid.Data3) << 48;

    for (int c = 0; c < 8; c++)
    {
        uuid2.hi() |= ((UBigInt)uuid.Data4[c]) << (c * 8);
    }

    return uuid2;
}

UUID UUID::make_random()
{
    WinUUID uuid;

    UuidCreate ( &uuid );

    return make_uuid(uuid);
}

UUID UUID::make_time()
{
    WinUUID uuid;

    UuidCreate ( &uuid );

    return make_uuid(uuid);
}


UUID UUID::parse(const char* in)
{
	WinUUID uu;

	unsigned char in_buffer[37];
	in_buffer[36] = 0;
	for (size_t c = 0; c < 36; c++) in_buffer[c] = in[c];

	UuidFromString(in_buffer, &uu);

    return make_uuid(uu);
}

std::ostream& operator<<(std::ostream& out, const UUID& uuid)
{
    WinUUID uu;

    uu.Data1 = uuid.lo();
    uu.Data2 = (uuid.lo() >> 32) & 0xFFFF;
    uu.Data3 = (uuid.lo() >> 48) & 0xFFFF;

    for (int c = 0; c < 8; c++)
    {
        uu.Data4[c] = uuid.lo() >> (c * 8);
    }


    unsigned char * str;
    UuidToStringA ( &uu, &str );

    out<< ( char* ) str;

    RpcStringFreeA ( &str );

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
