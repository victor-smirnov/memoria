
// Copyright Victor Smirnov 2012.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef _MEMORIA_TOOLS_CMDLINE_HPP
#define	_MEMORIA_TOOLS_CMDLINE_HPP

#include <memoria/core/types/types.hpp>
#include <memoria/core/tools/file.hpp>

#include <memoria/tools/configuration.hpp>

#include <vector>
#include <ostream>

namespace memoria {

using namespace std;

class CmdLine {
	int 			argc_;
	const char** 	argv_;
	const char** 	envp_;

	bool 			help_;
	bool 			dump_;


	String 			image_name_;
	String			cfg_file_name_;

	Configurator	cfg_file_;
	Configurator	cfg_cmdline_;

public:
	CmdLine(int argc, const char** argv, const char** envp, StringRef cfg_file_name):
		argc_(argc),
		argv_(argv),
		envp_(envp),
		help_(false),
		dump_(false),
		image_name_(GetImageName(argv[0])),
		cfg_file_name_(cfg_file_name),
		cfg_file_(),
		cfg_cmdline_(&cfg_file_)
	{
		Process();
	};

	StringRef GetConfigFileName() const
	{
		return cfg_file_name_;
	}

	StringRef GetImageName() const
	{
		return image_name_;
	}

	bool IsHelp()
	{
		return help_;
	}

	bool IsDump() {
		return dump_;
	}

	Configurator& GetConfigurator() {
		return cfg_cmdline_;
	}

	void Process();
protected:
	static String GetImagePathPart(const char* str);
	static String GetImageName(const char* str);
};


}
#endif
