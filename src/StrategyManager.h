#pragma once

#include "stdafx.h"
#include "ResourceManager.h"

class StrategyManager
{
	ResourceManager& resMgr;
public:
	StrategyManager(ResourceManager& RM)
		: resMgr(RM)
	{ }
	~StrategyManager() { }

	ErrCode execute();
};

