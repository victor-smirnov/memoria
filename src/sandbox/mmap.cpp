// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>

#include <memoria/containers/multimap/mmap_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memoria/core/tools/time.hpp>

#include <memory>
#include <vector>

using namespace memoria;
using namespace std;


int main() {
	MEMORIA_INIT(DefaultProfile<>);

	using KeyType = BigInteger;
	using ValueType = Byte;

	using CtrName = Map<KeyType, Vector<ValueType>>;


	DInit<CtrName>();


	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp   = alloc->master()->branch();

		auto map = create<CtrName>(snp);

	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	catch (memoria::PackedOOMException& ex) {
		cout << "PackedOOMException at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
