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


#include <memoria/v1/core/packed/sseq/packed_rle_searchable_seq.hpp>
#include <memoria/v1/core/tools/alloc.hpp>
#include <memoria/v1/core/tools/dump.hpp>

#include <memory>
#include <vector>

using namespace memoria::v1;
using namespace std;

int main()
{
    try {
        auto allocator1 = AllocateAllocator(65536);

        using Seq = PkdRLESeqT<4>;

        auto seq = allocator1->template allocateEmpty<Seq>(0);

        size_t sums[] = {0, 0, 0, 0};

        for (int c = 0; c < 1000; c++)
        {
            Int symbol = (c % 4);

            sums[symbol] += c + 1;

            seq->append(symbol, c + 1);
        }

        seq->reindex();
        seq->dump();
        seq->check();

        cout << endl << endl;

//      Int size = seq->size();
//      for (Int c = 0; c < size; c++)
//      {
//          auto iter = seq->selectFw(c, 1, 1);
//
//          cout << c << " -- " << iter.idx() << endl;
//      }

//      for (Int c = size - 1; c >= 0; c--)
//      {
//          auto iter = seq->selectBw(c, 1, 1);
//
//          if (iter.is_set())
//          {
//              cout << c << " -- " << iter.value().idx() << endl;
//          }
//          else {
//              cout << c << " -- OOR" << endl;
//          }
//      }


        cout << "Sums: " << sums[0] << " " << sums[1] << endl;
    }
    catch (::memoria::v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }
    catch (::memoria::v1::PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }
}
