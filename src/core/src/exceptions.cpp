#include <memoria/core/exceptions/memoria.hpp>

namespace memoria{ namespace vapi {

#ifndef MEMORIA_SRC
#error "MEMORIA_SRC symbol must be defined at compile time"
#endif

const char* ExtractMemoriaPath(const char* path) {

	const char* prefix = MEMORIA_TOSTRING(MEMORIA_SRC);

	Int c;
	for (c = 0; prefix[c] != '\0'; c++)
	{
		if (prefix[c] != path[c] && (path[c] != '\\' && path[c] !='/'))
		{
			return path;
		}
	}

	return path + c;
}

}}


namespace std {

using namespace memoria::vapi;

ostream& operator<<(ostream& out, MemoriaThrowable& t) {
	t.dump(out);
	return out;
}

}
