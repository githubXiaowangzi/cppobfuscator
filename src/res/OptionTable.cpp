#include "OptionTable.h"
#include "../stdafx.h"

using namespace std;

bool OptionTable::ParseArgs(int argc, char **argv) {
	DPRINT("argc = %d", argc);
	for (unsigned i = options::Invalid; i != options::EndOption; i++) {
		impl[i].clear();
	}

	string shortOpt;
	int choose;
	int longind;
	while ((choose = getopt_long(argc, argv, shortOpt.c_str(), options::InfoTable, &longind)) != 0) {
		switch (choose)
		{
		case options::OPT_Directory:
			assert(impl[choose].size() == 0 && "Can't output to multiple directories.");
			impl[choose].push_back(optarg);
			DPRINT("Output Directory: %s", optarg);
			break;
		case options::OPT_ScopeMode:
		case options::OPT_ScopeIncludeFlag:
		case options::OPT_ScopeExcludeFlag:
		case options::OPT_CustomObfsArg:
		case options::OPT_CustomClangArg:
		default:
			DPRINT("Option not implemented yet: [%s] [%s]", options::InfoTable[longind].name, optarg);
			break;
		}
	}

	// get input file name
	if (optind < argc) {
		while (optind < argc) {
			DPRINT("non-option argvs caught: [%s]", argv[optind]);
			impl[options::Argument].push_back(argv[optind++]);
		}
	}

	assert(impl[options::Argument].size() == 1 &&
		"Should set exactly 1 src file.");

	return true;
}

OptionTable::OptValueListTy& OptionTable::getOption(options::OPTID ID) {
	assert(ID >= options::Invalid && ID < options::EndOption
		&& "Option ID out of bound.");
	return impl[ID];
}