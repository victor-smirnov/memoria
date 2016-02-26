// Copyright Victor Smirnov 2015+.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>

#include <memoria/containers/map/map_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>



using namespace memoria;
using namespace std;

int main() {
	MEMORIA_INIT(DefaultProfile<>);

	DCtr<Map<BigInteger, BigInt>>::initMetadata();

	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp = alloc->master()->branch();

		auto map = create<Map<BigInteger, BigInt>>(snp);

		for (int c = -10000; c < 10000; c++) {
			map->assign(c, c);
		}

		for (auto c = map->begin(); !c->is_end(); c->next())
		{
			cout << c->key() << " -- " << c->value() << endl;
		}

		for (int c = -10000; c < 10000; c++) {
			map->remove(c);
		}

		cout << "After remove" << endl;

		for (auto c = map->begin(); !c->is_end(); c->next())
		{
			cout << c->key() << " -- " << c->value() << endl;
		}

		snp->commit();

		FSDumpAllocator(snp, "mapx.dir");
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
