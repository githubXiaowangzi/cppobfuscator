#pragma once

#include "Typedef.h"
#include "stdafx.h"
#include "res\OptionTable.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "llvm\Support\raw_ostream.h"
#include "llvm\Support\Host.h"
#include "llvm\Option\OptTable.h"

using namespace clang;
//using llvm::opt::OptTable;

class ResourceManager
{
protected:
	std::unique_ptr<CompilerInstance> compInst;
	std::unique_ptr<Rewriter> rw;
	std::unique_ptr<OptionTable> optTable;
	
	DeclGroupRefVec decls;
public:
	ResourceManager() { }
	~ResourceManager() {
		rw.reset();
		DPRINT("rw reset.");
		optTable.reset();
		DPRINT("optTable reset.");
		compInst.reset();
		DPRINT("compInst reset.");
	}

	void init(int argc, char **argv);

	bool initParseAST();
	
	void rewriteToFile();
	
	bool prettyPrint(/*llvm::raw_ostream& out*/);

	DeclGroupRef& updateAndGetDeclGroupRef();

	DeclGroupRef& getDeclGroupRef();

	int mkdirRecursively(const char *pathName, uint8_t mode = 0);

	inline std::string getRewriteFileName(std::string srcFileName) {
		srcFileName.insert(srcFileName.find_last_of("/\\") + 1, "@");
		return srcFileName;
	}
	inline CompilerInstance& getCompilerInstance() {
		return *compInst.get();
	}
	inline Rewriter& getRewriter() {
		return *rw.get();
	}
	inline OptionTable& getOptTable() {
		return *optTable.get();
	}
	inline DeclGroupRefVec& getDeclGroupRefVec() {
		return decls;
	}

};

