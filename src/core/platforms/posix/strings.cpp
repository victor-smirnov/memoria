
// Copyright Victor Smirnov 2011.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)




#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/file.hpp>

#include <errno.h>
#include <memoria/v1/core/tools/strings/string.hpp>
#include <stdlib.h>



namespace memoria {


using namespace std;

//Long StrToL(StringRef value) {
//  if (!isEmpty(value))
//  {
//      const char* ptr = trimString(value).c_str();
//      char* end_ptr;
//
//      errno = 0;
//      Long v = strtol(ptr, &end_ptr, 0);
//
//      if (errno == 0)
//      {
//          if (*end_ptr == '\0')
//          {
//              return v;
//          }
//          else {
//              throw Exception(MEMORIA_SOURCE, "Invalid integer value: " + value);
//          }
//      }
//      else {
//          throw Exception(MEMORIA_SOURCE, "Invalid integer value: " + value);
//      }
//  }
//  else {
//      throw Exception(MEMORIA_SOURCE, "Invalid integer value: " + value);
//  }
//}


}

