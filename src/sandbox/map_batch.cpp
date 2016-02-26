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
#include <vector>

using namespace memoria;
using namespace std;

template <typename T>
using SVector = shared_ptr<vector<T>>;

class MapEntryProvider {
	size_t pos_ = 0;
	shared_ptr<vector<String>> data_;
public:
	MapEntryProvider(const shared_ptr<vector<String>>& data): data_(data) {}
	MapEntryProvider() {}

	bool has_next() const {
		return pos_ < data_->size();
	}

	auto next()
	{
		auto entry = make_pair(data_->at(pos_), pos_);
		pos_++;

		return entry;
	}
};


int main() {
	MEMORIA_INIT(DefaultProfile<>);

	DCtr<Map<String, double>>::initMetadata();

	try {
		auto alloc = PersistentInMemAllocator<>::create();
		auto snp = alloc->master()->branch();

		auto map = create<Map<String, double>>(snp);

		auto data = make_shared<vector<String>>(1000000);

		for (size_t c = 0; c < data->size(); c++) {
			data->operator[](c) = "str_"+toString(c);
		}

		std::sort(data->begin(), data->end());

		map->begin()->insert_entries(MapEntryProvider(data));

		snp->commit();

		check_snapshot(snp);

		FSDumpAllocator(snp, "map_batch.dir");
	}
	catch (memoria::Exception& ex) {
		cout << ex.message() << " at " << ex.source() << endl;
	}

	MetadataRepository<DefaultProfile<>>::cleanup();
}
