#include "SimplePrinterConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/ParentMap.h"
#include "clang/Analysis/CFG.h"

using namespace clang;

class StmtPrinter : public RecursiveASTVisitor<StmtPrinter>
{
	CompilerInstance *compInst;
	std::unique_ptr<ParentMap> parMap;
public:
	StmtPrinter(CompilerInstance *CI, Stmt *S)
		: compInst(CI), parMap(new ParentMap(S)) { }

	bool VisitStmt(Stmt *S) {
		ASTContext& Ctx = compInst->getASTContext();
		DPRINT("Stmt %s ( %u -> p %u )", S->getStmtClassName(), 
			(unsigned int)S, (unsigned int)parMap.get() ? parMap->getParent(S) : 0);
		S->dump();
		S->dumpPretty(Ctx);
		NullStmt(SourceLocation()).dumpPretty(Ctx);
		return true;
	}
};

bool SimplePrinterConsumer::HandleTopLevelDecl(DeclGroupRef D)
{
	if (D.begin() == D.end())
		return true;

	Decl *firstD = *D.begin();
	if (compInst->getSourceManager().isInSystemHeader(firstD->getLocation()))
		return true;

	PrintingPolicy policy = compInst->getASTContext().getPrintingPolicy();
	NullStmt *nullSt = new (compInst->getASTContext())NullStmt(SourceLocation());

	for (DeclGroupRef::iterator I = D.begin(), E = D.end();
		I != E; I++) {
		Decl *dd = *I;

		DPRINT("PrintingPolicy: %d %d %d %d %d", policy.SuppressSpecifiers,
			policy.SuppressScope, policy.SuppressTagKeyword, policy.SuppressUnwrittenScope,
			policy.SuppressSpecifiers);

		dd->print(out, policy);
		nullSt->printPretty(out, nullptr, policy);
		if (dd->hasBody()) {
			Stmt *ss = dd->getBody();
			std::unique_ptr<CFG> cfg = CFG::buildCFG((const Decl *)dd, ss, &compInst->getASTContext(), CFG::BuildOptions());
			//cfg.reset(CFG::buildCFG((const Decl *)dd, ss, &compInst->getASTContext(), CFG::BuildOptions()));
			assert(cfg.get() != nullptr && "build CFG failed.");
			cfg->dump(compInst->getLangOpts(), true);
			cfg->viewCFG(compInst->getLangOpts());
		}
	}

	return true;
}
