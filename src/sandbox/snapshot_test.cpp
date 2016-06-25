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
#include <memoria/v1/containers/set/set_factory.hpp>
#include <memoria/v1/core/tools/iobuffer/io_buffer.hpp>

#include <memoria/v1/core/tools/strings/string_codec.hpp>
#include <memoria/v1/core/tools/time.hpp>
#include <memoria/v1/core/tools/fixed_array.hpp>
#include <memoria/v1/core/tools/random.hpp>
#include <memoria/v1/core/tools/ticker.hpp>

#include <algorithm>
#include <vector>
#include <type_traits>
#include <thread>
#include <mutex>

using namespace memoria::v1;
using namespace memoria::v1::btss;
using namespace std;

using MutexT = std::mutex;
using GuardT = lock_guard<MutexT>;

MutexT mutex_;

using Key = FixedArray<20>;
//using Key = Bytes;

class Creator {
	UUID ctr_name_;
	UUID snp_id_;
public:

	const UUID& ctr_name() const {return ctr_name_;}
	const UUID& snp_id() const {return snp_id_;}

	template <typename SnapshotT, typename Keys>
	void run(const SnapshotT& master, const Keys& keys, int th_num)
	{
		try {
			auto snp = master->branch();
			snp_id_  = snp->uuid();

			Ticker ticker(100000);

			Ticker batch(100000);

			auto ctr = create<Set<Key>>(snp);
			ctr_name_ = ctr->name();

			for (size_t c = 0; c < keys.size(); c++)
			{
				ctr->insert_key(keys[c]);

				if (ticker.is_threshold())
				{
					GuardT guard(mutex_);
					cout << "Thread " << th_num << ", ticks = " << (ticker.ticks() + 1) <<", time = " << ticker.duration() << endl;

					ticker.next();
				}

				if (batch.is_threshold())
				{
					snp->commit();

					{
						auto snp1 = snp;
						snp = snp->branch();
						snp_id_ = snp->uuid();

						ctr = find<Set<Key>>(snp, ctr_name_);

						snp1->drop();
					}

					snp->pack_allocator();

					batch.next();
				}

				ticker.tick();
				batch.tick();
			}

			snp->commit();

			GuardT guard(mutex_);
			cout << "Thread " << th_num << ", total time = " << (getTimeInMillis() - ticker.start_time()) << endl;
		}
		catch (Exception& ex) {
			GuardT guard(mutex_);
			cout << ex.message() << " at " << ex.source() << endl;
		}
	}
};


struct Checker {

	template <typename AllocatorT, typename Keys>
	void operator()(const AllocatorT& alloc, const Keys& keys, int th_num, const UUID& snp_id, const UUID& ctr_name)
	{
		try {
			BigInt t0 = getTimeInMillis();

			auto snp = alloc->find(snp_id);

			auto ctr = find<Set<Key>>(snp, ctr_name);

			for (size_t c = 0; c < keys.size(); c++)
			{
				if (!ctr->contains(keys[c]))
				{
					GuardT guard(mutex_);
					cout << "Snapshot " << snp_id << " ctr " << ctr_name << " doesn't has key " << keys[c] << endl;
				}
			}

			GuardT guard(mutex_);
			cout << "Check thread " << th_num << ", total time = " << (getTimeInMillis() - t0) << endl;
		}
		catch (Exception& ex) {
			GuardT guard(mutex_);
			cout << ex.message() << " at " << ex.source() << endl;
		}
	}
};


int main()
{
    MEMORIA_INIT(DefaultProfile<>);

    DInit<Set<Key>>();

    try {
        auto alloc = PersistentInMemAllocator<>::create();

        size_t total_keys = 300000;
        int thread_num = 4;

        vector<Key> keys;

        for (size_t c = 0; c < total_keys; c++)
        {
        	Key array;//(20);

        	for (int c = 0; c < array.length(); c++)
        	{
        		array[c] = getRandomG(256);
        	}

        	keys.emplace_back(array);
        }

        vector<unique_ptr<Creator>> creators;
        vector<thread> producers;

        {
        	GuardT guard(mutex_);
        	cout << "Starting creator threads" << endl;
        }

        auto master = alloc->master();

        for (Int c = 0; c < thread_num; c++)
        {
        	creators.emplace_back(make_unique<Creator>());
        }

        for (Int c = 0; c < thread_num; c++)
        {
        	producers.emplace_back(thread([&](auto&& s, auto&& k, auto n){
        		creators[n]->run(s, k, n);
        	}, master, keys, c));
        }

        {
        	GuardT guard(mutex_);
        	cout << "Joining creator threads" << endl;
        }

        for (auto& th: producers)
        {
        	th.join();
        }

        {
        	GuardT guard(mutex_);
        	cout << "Importing data" << endl;
        }

        auto snp = alloc->master()->branch();
        snp->lock_data_for_import();

        for (const auto& cr: creators)
        {
        	auto ss = alloc->find(cr->snp_id());
        	snp->import_ctr_from(ss, cr->ctr_name());
        	ss->drop();
        }

        snp->commit();
        snp->set_as_master();

        vector<thread> checkers;

        for (Int c = 0; c < thread_num; c++)
        {
        	checkers.emplace_back(thread(Checker(), alloc, keys, c, alloc->master()->uuid(), creators[c]->ctr_name()));
        }

        for (auto& th: checkers)
        {
        	th.join();
        }

        cout << "Store data..." << endl;

        // Store binary contents of allocator to the file.

        BigInt ts0 = getTimeInMillis();
        alloc->store("snapshots_mt.dump");

        {
        	GuardT guard(mutex_);
        	cout << "Store time: " << (getTimeInMillis() - ts0) << endl;
        }

        BigInt tl0 = getTimeInMillis();
        auto alloc1 = PersistentInMemAllocator<>::load("snapshots_mt.dump");
        cout << "Load time: " << (getTimeInMillis() - tl0) << endl;

        alloc->dump("snapshots_mt.dir");

        vector<thread> checkers1;

        for (Int c = 0; c < thread_num; c++)
        {
        	checkers1.emplace_back(thread(Checker(), alloc1, keys, c, alloc1->master()->uuid(), creators[c]->ctr_name()));
        }

        for (auto& th: checkers1)
        {
        	th.join();
        }

        {
        	GuardT guard(mutex_);
        	cout << "Done..." << endl;
        }
    }
    catch (Exception& ex)
    {
    	GuardT guard(mutex_);
        cout << ex.message() << " at " << ex.source() << endl;
    }

    // Destroy containers metadata.
    MetadataRepository<DefaultProfile<>>::cleanup();
}
