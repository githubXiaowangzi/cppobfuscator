#pragma once

#include "../Algorithm.h"
#include "ASTTraverserPlus.h"

class VarRenamer : public Algorithm, public ASTTraverserPlus<VarRenamer>
{
public:
	VarRenamer(ResourceManager& RM)
		: Algorithm(RM) { }
	
	bool HandleDecl(Decl *D);

	bool VisitDecl(Decl *&D);
};
