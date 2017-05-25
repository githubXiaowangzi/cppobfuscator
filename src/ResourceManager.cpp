#include "ResourceManager.h"
#include "HeaderSearchPath.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "llvm/ADT/DenseSet.h"

void ResourceManager::init(int argc, char **argv) {
	decls.clear();
	compInst.reset();
	rw.reset();
	optTable.reset();

	optTable.reset(new OptionTable(argc, argv));

	compInst.reset(new CompilerInstance());
	CompilerInvocation& compInvo = compInst->getInvocation();
	HeaderSearchOptions& hsOpts = compInst->getHeaderSearchOpts();
	hsOpts.UseBuiltinIncludes = 1;
	hsOpts.UseStandardSystemIncludes = 1;
	hsOpts.UseStandardCXXIncludes = 1;
	hsOpts.Verbose = 1;

	for (int i = 0; i < sizeof(HS_PATHS) / sizeof(string); i++) {
		hsOpts.AddPath(HS_PATHS[i], clang::frontend::CXXSystem, false, false);
	}
	compInvo.setLangDefaults(compInst->getLangOpts(), IK_CXX, clang::LangStandard::lang_cxx98);
	compInst->createDiagnostics(nullptr, false);

	TargetOptions tarOpts;
	tarOpts.Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *tarInfo = TargetInfo::CreateTargetInfo(compInst->getDiagnostics(), &tarOpts);
}