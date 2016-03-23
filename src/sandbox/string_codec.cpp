
// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/v1/core/tools/time.hpp>

#include <memoria/v1/core/tools/strings/string.hpp>
#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/assert.hpp>

#include <iostream>
#include <vector>
#include <cstddef>
#include <memory>

using namespace memoria;
using namespace std;

int main()
{
    try {
        size_t block_size = 1024*1024ull;

        unique_ptr<UByte[]> buf = make_unique<UByte[]>(block_size);

        ValueCodec<String> codec;

        size_t cnt = 0;
        for (size_t pos = 0; pos < block_size - 100; cnt++)
        {
            String value = toString(pos);

            auto len = codec.encode(buf.get(), value, pos);

            try {
                MEMORIA_V1_ASSERT(len, ==, codec.length(value));
                MEMORIA_V1_ASSERT(len, ==, codec.length(buf.get(), pos, -1ull));
            }
            catch(...) {
                cout << "Value: " << value << endl;
                throw;
            }

            pos += len;
        }

        for (size_t pos = 0, c = 0; c < cnt; c++)
        {
            String val;
            auto len = codec.decode(buf.get(), val, pos);

            MEMORIA_V1_ASSERT(val, ==, toString(pos));

            pos += len;
        }
    }
    catch (Exception& ex)
    {
        cout<<ex.source()<<endl;
        cout<<ex<<endl;
    }
}
