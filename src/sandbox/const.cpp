

#include <memoria/memoria.hpp>

#include <memoria/core/tools/elias_codec.hpp>
#include <memoria/core/tools/i64_codec.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

using namespace std;
using namespace memoria;
using namespace memoria::core;



int main(void) {

	size_t lmax = 0;

	for (BigInt c = 0; c < 65536; c++)
	{
		size_t len = GetI64ValueLength(c);

		if (len < lmax) {
			cout<<c<<" "<<len<<" "<<lmax<<endl;
			break;
		}
		else {
			cout<<c<<" "<<len<<endl;
		}
	}

    return 0;
}

