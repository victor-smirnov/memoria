
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

        unique_ptr<uint8_t[]> buf = make_unique<uint8_t[]>(block_size);

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
