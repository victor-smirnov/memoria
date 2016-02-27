// Copyright Victor Smirnov 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <memoria/memoria.hpp>

#include <memoria/containers/map/map_factory.hpp>
#include <memoria/containers/seq_dense/seqd_factory.hpp>
#include <memoria/containers/vector/vctr_factory.hpp>

#include <memoria/core/container/metadata_repository.hpp>

#include <memory>

using namespace memoria;
using namespace std;




int main() {
	MEMORIA_INIT(DefaultProfile<>);

	using Key 	= BigInt;
	using Value = BigInt;

	DInit<Map<Key, Value>>();

	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp = alloc->master()->branch();

		auto map = create<Map<Key, Value>>(snp);

		for (int c = -10000; c < 10000; c++) {
//			map->assign(toString(c), c);
			map->assign(c, c);
		}

		MEMORIA_ASSERT(map->size(), ==, 20000);

		check_snapshot(snp);

		for (auto c = map->begin(); !c->is_end(); c->next())
		{
			cout << c->key() << " -- " << c->value() << endl;
		}


		for (int c = -10000; c < 10000; c++) {
//			map->remove(toString(c));
			map->remove(c);
		}

		cout << "After remove" << endl;

		for (auto c = map->begin(); !c->is_end(); c->next())
		{
			cout << c->key() << " -- " << c->value() << endl;
		}

		MEMORIA_ASSERT(map->size(), ==, 0);

		snp->commit();

		check_snapshot(snp);

		FSDumpAllocator(snp, "mapx.dir");
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
