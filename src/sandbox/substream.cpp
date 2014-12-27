


#include <memoria/prototypes/bt/tools/bt_packed_struct_list_builder.hpp>
#include <memoria/prototypes/bt/tools/bt_size_list_builder.hpp>

#include <memoria/core/tools/type_name.hpp>
#include <memoria/core/types/list/list_tree.hpp>

#include <typeinfo>
#include <iostream>
#include <vector>

template <int I>
struct S {};

namespace memoria {
namespace bt {

template <Int I>
struct StructSizeProvider<S<I>> {
    static const Int Value = I;
};

}
}


using namespace std;
using namespace memoria;
using namespace memoria::core;
using namespace memoria::bt;
using namespace memoria::list_tree;

int main(void)
{

    return 0;
}

