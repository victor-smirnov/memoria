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



#include <memoria/v1/memoria.hpp>

#include <memoria/v1/containers/labeled_tree/ltree_factory.hpp>

#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/random.hpp>

#include <memory>
#include <vector>

using namespace memoria::v1;
using namespace std;


int main() {
    MEMORIA_INIT(DefaultProfile<>);

    using CtrName = LabeledTree<
            FLabel<uint16_t>,
            VLabel<int64_t, Granularity::Bit, Indexed::Yes>
    >;

    using CtrT = DCtrTF<CtrName>::Type;

    using LabelsTuple = CtrT::Types::LabelsTuple;

    DInit<CtrName>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();
        auto snp   = alloc->master()->branch();
        try {
            auto ctr = create<CtrName>(snp);

            ctr->select0(1);

            ctr->newNodeAt(louds::LoudsNode(0, 0), LabelsTuple(0, 0));
            ctr->remove(0);
        }
        catch (...) {
            FSDumpAllocator(snp, "mmap_fail.dir");
            throw;
        }
    }
    catch (Exception& ex) {
        cout << ex.message() << " at " << ex.source() << endl;
    }
    catch (PackedOOMException& ex) {
        cout << "PackedOOMException at " << ex.source() << endl;
    }

    MetadataRepository<DefaultProfile<>>::cleanup();
}
