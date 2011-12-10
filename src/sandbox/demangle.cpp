#ifdef __GNUC__
#include <cxxabi.h>
#endif

#include <iostream>
#include <iomanip>
using namespace std;


int main(void) {

#ifdef __GNUC__
	const int BufferSize = 40960;
	const char* text = "_ZN7memoria5btree19NodeDispatcherTool0INS0_32BTreeContainerDispatchersBuilderINS_4core5CtrTFINS_13StreamProfileIvEENS_5BTreeENS_5KVMapIxxEEE15DispatcherTypesEE9RootTypesELi0EE8DispatchINS3_7CtrPartINS0_9ToolsNameENS3_9CtrHelperILi4ENS3_9CtrTypesTINSA_5TypesEEEEESL_E10MetadataFnILb1EEEEEvPNSA_16NodePageBaseTypeERT_";

	char buf[BufferSize];
	size_t len = sizeof(buf);
	abi::__cxa_demangle(text, buf, &len, NULL);
	cout<<buf<<endl;
#endif

	cout<<left<<setw(20);
	cout<<"01234567890";
	cout<<left<<setw(20);
	cout<<"01234567890"<<"X"<<endl;

	return 0;
}
