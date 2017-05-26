#pragma once

#include "../Algorithm.h"

class FlattenCFGTransformer : public Algorithm
{
public:
	FlattenCFGTransformer(ResourceManager& RM) 
		: Algorithm(RM) { }

	virtual bool execute();
};