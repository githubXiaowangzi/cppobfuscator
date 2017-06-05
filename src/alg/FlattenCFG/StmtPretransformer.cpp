#include "StmtPretransformer.h"

using namespace clang;

bool StmtPretransformer::HandleDecl(Decl *D) {
	DPRINT("START StmtPretransformer");
	assert(isa<FunctionDecl>(D) && "not function decl");

	if (!D->hasBody()) {
		DPRINT("no function body");
		return true;
	}

	this->stmtStack.clear();
	this->stmtMap.clear();
	this->parMap = new ParentMap(D->getBody());
	this->declStack.clear();

	this->TraverseDecl(D);

	this->stmtStack.clear();
	while (!stmtMap.empty()) {
		delete stmtMap.begin()->second;
		stmtMap.erase(stmtMap.begin());
	}
	delete this->parMap;
	this->parMap = nullptr;
	this->declStack.clear();
	DPRINT("END StmtPretransformer");

	return true;
}

bool StmtPretransformer::VisitDecl(Decl *D) {
	DPRINT("visitDecl %x(%s) Ctx %x", 
		(unsigned)D, D->getDeclKindName(), (unsigned)D->getDeclContext());

	if (isa<FunctionDecl>(D)) {
		declStack.push_back(D);
	}
	return true;
}

bool StmtPretransformer::ExitDecl(Decl *D) {
	DPRINT("exit decl");

	if (isa<FunctionDecl>(D)) {
		declStack.pop_back(D);
	}
	return true;
}

bool StmtPretransformer::VisitStmt(Stmt *S) {
	StmtPretransInfo *pInfo = new StmtPretransInfo(S, &S, nullptr);
	stmtStack.push_back(pInfo);
	stmtMap[S] = pInfo;
	if (!this->parMap->getParent(S)) {
		this->parMap->addStmt(S);
	}

	DPRINT("push stmt %s %x %x %x", 
		S->getStmtClassName(), (unsigned int)S, stmtStack.back(), stmtMap[S]);
	return true;
}

bool StmtPretransformer::ExitStmt(Stmt *S) {
	DPRINT("leaving Stmt %s, start transform", S->getStmtClassName());
	S->dump();
	S->dumpPretty(resMgr.getCompilerInstance().getASTContext());
	switch (S->getStmtClass())
	{
	case Stmt::IfStmtClass:
	{
		ExtractIfCondVarDecl(dyn_cast<IfStmt>(S));
		break;
	}
	case Stmt::WhileStmtClass:
	{
		WhileToIf(S);
		break;
	}
	case Stmt::DoStmtClass:
	{
		DoToIf(S);
		break;
	}
	case Stmt::ForStmtClass:
	{
		ForToIf(S);
		break;
	}
	case Stmt::SwitchStmtClass:
	{
		SwitchToIf(S);
		break;
	}
	case Stmt::MemberExprClass:
	{
		if (MemberExpr *ME = dyn_cast<MemberExpr>(S)) {
			if (Expr *Base = ME->getBase()) {
				DPRINT("add paran to MemberExpr");
				Expr *PE = BuildParenExpr(Base);
				ME->setBase(PE);
				parMap->addStmt(ME);
			}
		}
		break;
	}
	default:
		return true;
	}

	return true;
}

