
// Copyright Victor Smirnov 2015.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <functional>

using namespace std;

class Consumer {
	int size_;
public:
	Consumer(int size): size_(size) {}

	bool accept(int length) {
		cout<<"Accept: "<<length<<endl;
		return size_ >= length;
	}

	int size() const {
		return size_;
	}
};



int InsertData(Consumer& consumer, int start, int size)
{
	if (consumer.accept(size - start)) {
		return size - start;
	}
	else {
		int imax = size, imin = start;
		int s0 = 0, accepts = 0;

		while (accepts < 5 && imax >= imin)
		{
			int mid = imin + ((imax - imin) / 2);

			if (consumer.accept(s0 + mid - imin))
			{
				accepts++;
				s0 += mid - imin;
				cout<<"S0: "<<s0<<endl;
				imin = mid + 1;
			}
			else {
				imax = mid - 1;
			}
		}

		return s0;
	}
}

int main(void)
{
	Consumer consumer(16331);

	cout<<InsertData(consumer, 123, 1000000 + 124)<<"\n"<<consumer.size()<<endl;
}
