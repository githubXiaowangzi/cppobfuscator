#include "StrategyManager.h"
#include "alg/Algorithm.h"
#include "alg/FlattenCFG/FlattenCFGTransformer.h"
#include "alg/SimplePrinter.h"

ErrCode StrategyManager::execute()
{
	resMgr.initParseAST();
	FlattenCFGTransformer(resMgr).execute();
	SimplePrinter(resMgr).execute();

	return 0;
}
