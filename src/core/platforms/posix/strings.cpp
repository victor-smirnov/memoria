
// Copyright 2011 Victor Smirnov
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





#include <memoria/v1/core/exceptions/exceptions.hpp>
#include <memoria/v1/core/tools/file.hpp>

#include <errno.h>
#include <memoria/v1/core/tools/strings/string.hpp>
#include <stdlib.h>



namespace memoria {
namespace v1 {


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


}}