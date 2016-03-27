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

#include <memoria/v1/containers/wt/wt_factory.hpp>
#include <memoria/v1/core/tools/random.hpp>

using namespace memoria::v1;
using namespace std;


vector<UInt> createRandomAlphabet(Int size)
{
	vector<UInt> text(size);

	for (auto& v: text)
	{
		v = getRandomG();
	}

	return text;
}


vector<UInt> createRandomText(Int size, const vector<UInt>& alphabet)
{
	vector<UInt> text(size);

	for (auto& v: text)
	{
		v = alphabet[getRandomG(alphabet.size())];
	}

	return text;
}



int main()
{
    // Initialize common metadata. Every Memoria program must use this macro.
    MEMORIA_INIT(DefaultProfile<>);

    // Every Memoria container has some static metadata used for I/O. Before a container is instantiated
    // this metadata must be initialized.
    DInit<WT>();

    try {
        // Create persistent in-memory allocator for containers to store their data in.
        auto alloc = PersistentInMemAllocator<>::create();

        // This callocator is persistent not because it can dump its content to stream or file, but in a
        // narrow sense: to make a group of changes to allocator one must first start new snapshot by
        // branching form any exiting one's.

        // Snapshots are completely isolated from each other, operations don't interfere with operations
        // on other snapshots.

        auto snp = alloc->master()->branch();

        // Create Wavelet Tree
        auto ctr = create<WT>(snp);

        ctr->prepare();

        Int alphabet_size = 100;
        Int text_size = 50;


        auto alphabet = createRandomAlphabet(alphabet_size);
        auto text = createRandomText(text_size, alphabet);

        for (UInt c = 0; c < text.size(); c++)
        {
            cout << c << " " << hex << text[c] << dec << endl;

            UBigInt value1 = text[c];

            ctr->insert(c, value1);
        }

        FSDumpAllocator(snp, "wt_dump.dir");

        ctr->drop();

        // Finish snapshot so no other updates are possible.
        snp->commit();


        // Store binary contents of allocator to the file.
        auto out = FileOutputStreamHandler::create("wt_data.dump");
        alloc->store(out.get());
    }
    catch (Exception& ex)
    {
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
