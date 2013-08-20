// Copyright Victor Smirnov 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <memoria/core/tools/terminal.hpp>

#include <unistd.h>
#include <string>
#include <iostream>

namespace memoria 	{
namespace tools 	{

using namespace std;

const TermImpl* Term::term_;



class LinuxTerminal: public TermImpl {
	virtual const char* reds() const 	{return "\033[1;31m";}
	virtual const char* rede() const 	{return "\033[0m";}

	virtual const char* greens() const 	{return "\033[1;32m";}
	virtual const char* greene() const 	{return "\033[0m";}
};

MonochomeTerminal 	mono_terminal_;
LinuxTerminal 		linux_terminal_;


void Term::init(int argc, const char** argv, const char** envp)
{
	bool color_term = false;

	for (;*envp; envp++)
	{
		string entry(*envp);

		if (entry == "TERM=linux" || entry == "TERM=xterm")
		{
			color_term = true;
			break;
		}
	}

	if (color_term && isatty(1))
	{
		term_ = & linux_terminal_;
	}
	else {
		term_ = & mono_terminal_;
	}
}


}
}
