
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)



#include <memoria/vapi.hpp>

#include <string>

using namespace std;
using namespace memoria::vapi;

namespace memoria {

Int PageCtrCnt[10] = {0,0,0,0,0,0,0,0,0,0};
Int PageDtrCnt[10] = {0,0,0,0,0,0,0,0,0,0};

Int PageCtr = 0;
Int PageDtr = 0;

bool GlobalDebug = false;


namespace vapi {


MEMORIA_EXPORT void InitTypeSystem(int argc, const char** argv, const char** envp, bool read_config_files) {

}

MEMORIA_EXPORT void DestroyTypeSystem() {

}

}}
