#pragma once

#include "../stdafx.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_os_ostream.h"

class SimplePrinterConsumer : public clang::ASTConsumer
{
	llvm::raw_ostream &out;
	clang::CompilerInstance *compInst;

public:
	SimplePrinterConsumer(llvm::raw_ostream& O, clang::CompilerInstance *CI)
		: out(O), compInst(CI) { }

	~SimplePrinterConsumer() { }

	virtual bool HandleTopLevelDecl(clang::DeclGroupRef D);
};