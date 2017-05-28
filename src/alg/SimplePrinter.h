#pragma once

#include "Algorithm.h"

class SimplePrinter : public Algorithm
{
public:
	SimplePrinter(ResourceManager& RM)
		: Algorithm(RM) { }

	virtual bool execute();
};