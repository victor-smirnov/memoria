
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

	int 			operations_;

	Configurator	replay_file_;
	bool			replay_;
	String			dump_file_name_;

public:

	enum {REPLAY = 1};

	CmdLine(int argc, const char** argv, const char** envp, StringRef cfg_file_name, int operations = 0):
		argc_(argc),
		argv_(argv),
		envp_(envp),
		help_(false),
		dump_(false),
		image_name_(GetImageName(argv[0])),
		cfg_file_name_(cfg_file_name),
		cfg_file_(),
		cfg_cmdline_(&cfg_file_),
		operations_(operations),
		replay_(false)
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

	StringRef GetDumpFileName() const
	{
		return dump_file_name_;
	}

	bool IsHelp()
	{
		return help_;
	}

	bool IsDump() {
		return dump_;
	}

	bool IsReplay() {
		return replay_;
	}

	Configurator& GetConfigurator()
	{
		return cfg_cmdline_;
	}

	Configurator& GetReplayOperationConfigurator()
	{
		return replay_file_;
	}

	void Process();
protected:
	static String GetImagePathPart(const char* str);
	static String GetImageName(const char* str);
};


}
#endif
