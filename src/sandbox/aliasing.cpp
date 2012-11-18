
#include <memoria/containers/root/factory.hpp>
#include <memoria/allocators/ct-test/factory.hpp>

#include <iostream>

#include <math.h>

using namespace std;
using namespace memoria;

using namespace memoria::vapi;
using namespace memoria::btree;

typedef T2TBuf<int*, unsigned short*> I2S;

int main(void) {

    const int C = 0xAABBCCDD;
    const int C1 = 0xCCDDAABB;

    int i1 = C;

    I2S i2s(&i1);

    unsigned short tmp = i2s.dst[0];

    i2s.dst[0] = i2s.dst[1];

    i2s.dst[1] = tmp;

    int i2 = C;

    unsigned short* sptr = (unsigned short*)&i2;

    tmp = sptr[0];

    sptr[0] = sptr[1];

    sptr[1] = tmp;

    if (i1 != C1 || i2 != C1)
    {
        cout<<"The machine code is broken due to strict aliasing rule"<<endl;
        cout<<"Via Union: "<<hex<<i1<<endl;
        cout<<"Via Cast:  "<<hex<<i2<<endl;
    }
    else {
        cout<<"The machine code is NOT broken via strict aliasing rule"<<endl;
    }

    double val = -sqrt((sqrt(5.0) - 1.0)/2.0);

    cout<<(val*val*val*val + val*val)<<" "<<val<<endl;

    return 0;
}
