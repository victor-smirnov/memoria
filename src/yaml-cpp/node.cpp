#include <memoria/v1/yaml-cpp/node/node.h>
#include "nodebuilder.h"
#include "nodeevents.h"

namespace memoria {
namespace v1 {
namespace YAML {
Node Clone(const Node& node) {
  NodeEvents events(node);
  NodeBuilder builder;
  events.Emit(builder);
  return builder.Root();
}
}
}}
