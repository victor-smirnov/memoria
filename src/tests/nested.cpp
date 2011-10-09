/*
 * nested.cpp
 *
 *  Created on: 20.09.2011
 *      Author: Victor
 */

#include <iostream>

using namespace std;

template <typename Selector>
struct Main {
	typedef int BooType;

	template <int a> struct A {
		static void dump() {
			BooType b;
			cout<<"first "<<b<<endl;
		}
	};
};


template <>
struct Main<int> {
	template <typename BooType> struct A {
		static void dump() {
			BooType b;
			cout<<"third "<<b<<endl;
		}
	};
};


template <>
template <>
struct Main<int>::A<int> {
	static void dump() {
		int b;
		cout<<"second "<<b<<endl;
	}
};


int main(void) {
	Main<int>::A<int>::dump();
}
