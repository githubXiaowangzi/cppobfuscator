#pragma once
#include "../stdafx.h"
#include "../Typedef.h"

#include "clang/AST/Decl.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"

class InitParseConsumer : public clang::ASTConsumer
{
protected:
	DeclGroupRefVec& decls;
	clang::CompilerInstance *compInst;
public:
	InitParseConsumer(DeclGroupRefVec &DV, clang::CompilerInstance *CI)
		: decls(DV), compInst(CI) { }
	~InitParseConsumer() { }

	virtual bool HandleTopLevelDecl(clang::DeclGroupRef DR) {
		clang::Decl *firstD = *DR.begin();
		if (compInst->getSourceManager().isInSystemHeader(firstD->getLocation())) {
			return true;
		}

		decls.push_back(DR);
		DPRINT("+decl, size = %d", decls.size());
		return true;
	}
};

