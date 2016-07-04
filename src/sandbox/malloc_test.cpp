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


#include <memoria/v1/core/tools/ticker.hpp>

#include <algorithm>
#include <vector>
#include <type_traits>
#include <thread>
#include <mutex>
#include <future>
#include <iostream>
#include <jemalloc/jemalloc.h>

using namespace memoria::v1;
using namespace std;

using MutexT = std::mutex;
using GuardT = lock_guard<MutexT>;

MutexT mutex_;

struct FHolderBase {
	virtual void get() = 0;
};

template <typename F>
struct FutureHolder: FHolderBase {
	F f_;

	FutureHolder(F&& f): f_(f) {}

	virtual void get() {
		f_.get();
	}
};

int main()
{
	vector<thread> threads;
	int threads_num = 4;
	for (int c = 0; c < threads_num; c++)
	{
		threads.emplace_back(thread([](int th_num){
			Ticker ticker(10000000);

			for (int n = 0; n < 20000000; n++) {
				void* mem = malloc(n % 10 + 20);

				if (ticker.is_threshold())
				{
					GuardT guard(mutex_);
					cout << "Thread " << th_num << ", ticks = " << (ticker.ticks() + 1) <<", time = " << ticker.duration() << " -- " << mem << endl;

					ticker.next();
				}

				ticker.tick();
			}

		}, c));
	}

	for (auto& th: threads) {
		th.join();
	}
}
