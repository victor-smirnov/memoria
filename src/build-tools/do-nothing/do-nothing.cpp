#include <iostream>
#include <fstream>

#include <string.h>

using namespace std;

int main(int argc, char** argv) {

	if (argc > 1 && strcmp("cr", argv[1]) == 0)
	{
		for (int arg = 2; arg < argc; arg++)
		{
			char buf[1024];
			strcpy(buf, argv[arg]);
#ifdef __WINNT__
			for (int i = 0; i < strlen(buf); i++)
			{
				if (buf[i] == '/') buf[i] = '\\';
			}
#endif
			fstream f;
			f.open(buf, ios::out);
			f<<"dummy";
			f.close();
//			cout<<arg<<" "<<argv[arg]<<endl;
		}
	}
	else {
		for (int arg = 2; arg < argc; arg++)
		{
//			cout<<arg<<" "<<argv[arg]<<endl;
		}
	}

	return 0;
}

