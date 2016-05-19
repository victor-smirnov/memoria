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

		using Seq = PkdRLESeqT<2>;

		auto seq = allocator1->template allocateEmpty<Seq>(0);

		seq->append(0, 1000);
		seq->reindex();
		seq->dump();
		cout << endl << endl;

		cout << endl << endl;
		seq->set_symbol(1, 1);
		seq->dump();

		cout << endl << endl;
		seq->set_symbol(1, 0);
		seq->dump();

//
//		seq->insert(1, 1, 10);
//		seq->dump();
//
//		cout << endl << endl;
//
//		seq->insert(1010, 0, 5);
//		seq->dump();

	}
    catch (::memoria::v1::Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }
    catch (::memoria::v1::PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }
}
