#include <memoria/v1/yaml-cpp/null.h>

namespace memoria {
namespace v1 {
namespace YAML {
_Null Null;

bool IsNullString(const std::string& str) {
  return str.empty() || str == "~" || str == "null" || str == "Null" ||
         str == "NULL";
}
}
}}
