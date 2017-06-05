#pragma once

#include "../Algorithm.h"
#include "ASTTraverserPlus.h"

#include "clang/AST/ParentMap.h"

using llvm::SmallVector;
using llvm::DenseMap;

class StmtPretransInfo
{
public:
	Stmt *stOrig;	//original stmt in AST
	Stmt **pInEdge;	//original stmt's parent's ptr that point in
	Stmt *stNew;	//new stmt to replace old one

public:
	StmtPretransInfo(Stmt *O, Stmt **E, Stmt *N)
		: stOrig(O), pInEdge(E), stNew(N) { }
};

class StmtPretransformer : public Algorithm, public ASTTraverserPlus<StmtPretransformer>
{
public:
	typedef StmtPretransInfo *StmtPretransInfoPtrTy;
	typedef SmallVector<StmtPretransInfoPtrTy, 32> StmtNodeSmallVector;
	typedef DenseMap<Stmt *, StmtPretransInfoPtrTy> StmtNodeMap;

	StmtPretransformer(ResourceManager& RM) : Algorithm(RM) { }

	bool HandleDecl(Decl *D);

	bool VisitStmt(Stmt *S);

	bool ExitStmt(Stmt *S);

	bool VisitDecl(Decl *D);

	bool ExitDecl(Decl *D);

protected:
	StmtNodeSmallVector stmtStack;	//point to the Stmt * which point to a child
	DeclPtrSmallVector declStack;	//point to the neariest enclosing Decl
	StmtNodeMap stmtMap;
	ParentMap *parMap;

protected:
	//extract vardecl in ifcond to avoid dumpPretty bug:
	//if(T t=x){..}  to   T t=x; if(t){..}
	bool ExtractIfCondVarDecl(IfStmt *S);

	bool WhileToIf(Stmt *S);

	bool DoToIf(Stmt *S);

	bool ForToIf(Stmt *S);

	bool SwitchToIf(Stmt *S);

	bool InnerJumpToGoto(const Stmt *stRoot, LabelStmt *stLblContinue, LabelStmt *stLblBreak);

	//update stmtMap.pInEdge of children of S
	bool updateChildrenInEdg(Stmt *S);

};