#include <memoria/memoria.hpp>
#include <memoria/tools/cmdline.hpp>
#include <memoria/tools/task.hpp>

#include <iostream>

using namespace std;
using namespace memoria;

MEMORIA_INIT();

const char* DESCRIPTION = "Run Memoria regression tests with specified configuration";
const char* CFG_FILE	= "tests.properties";

int main(int argc, const char** argv, const char** envp)
{
	try {
		CmdLine cmd_line(argc, argv, envp, CFG_FILE);

		TaskRunner runner;

		// add tasks to the runner;

		runner.Configure(&cmd_line.GetConfigurator());

		if (cmd_line.IsHelp())
		{
			cout<<endl;
			cout<<"Description: "<<DESCRIPTION<<endl;
			cout<<"Usage: "<<cmd_line.GetImageName()<<" [--help] [--dump] [--config <file.properties>]"<<endl;
			cout<<"    --help                     Display this help"<<endl;
			cout<<"    --config <file.properties> Use the specified config file"<<endl;
			cout<<"    --dump                     Dump available tasks and their configuration properties"<<endl;
		}
		else if (cmd_line.IsDump())
		{
			runner.DumpProperties(cout);
		}
		else {
			runner.Run(cout);
			cout<<"Done..."<<endl;
		}
	}
	catch (MemoriaException e)
	{
		cerr<<"ERROR: "<<e.message()<<endl;
	}
}
