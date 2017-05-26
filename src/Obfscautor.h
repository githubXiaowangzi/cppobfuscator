#pragma once
#include "stdafx.h"
#include "ResourceManager.h"
#include "StrategyManager.h"

class Obfscautor
{
	std::unique_ptr<ResourceManager> resMgr;
	std::unique_ptr<StrategyManager> staMgr;

public:
	inline void init() {
		resMgr.reset(new ResourceManager());
		staMgr.reset(new StrategyManager(*resMgr));
	}

	bool doit(int argc, char **argv) {
		ResourceManager& RM = *resMgr.get();
		RM.init(argc, argv);
		StrategyManager& SM = *staMgr.get();
		//SM
		RM.rewriteToFile();
		return true;
	}
};

