#pragma once

#include <getopt.h>
#include <vector>
#include <string>

using std::vector;
using std::string;

namespace options {
	enum OPTID {
		Invalid = 0,
#define OPTION(NAME, HAS_ARG, FLAG, ID, SHORT, HELPTEXT)	\
		OPT_##ID,
#include "OptionTable.inc"
		Argument,
		EndOption
#undef OPTION
	};

	static option InfoTable[] = {
#define OPTION(NAME, HAS_ARG, FLAG, ID, SHORT, HELPTEXT) \
	{NAME, HAS_ARG, FLAG, OPT_##ID},
#include "OptionTable.inc"
#undef OPTION
	{0, 0, 0, 0}
	};
}

class OptionTable
{
public:
	typedef vector<string> OptValueListTy;
	typedef vector<OptValueListTy> OptPoolTy;

protected:
	OptPoolTy impl;

public:
	OptionTable(int argc, char **argv) : impl(OptPoolTy(options::EndOption)) {
		this->ParseArgs(argc, argv);
	}

	bool ParseArgs(int argc, char **argv);

	OptValueListTy& getOption(options::OPTID ID);
};

