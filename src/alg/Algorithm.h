#pragma once
#include "../ResourceManager.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/Sema/Sema.h"
#include "llvm/ADT/SmallVector.h"

using llvm::SmallVector;
using namespace clang;

typedef SmallVector<Stmt *, 32> StmtPtrSmallVector;
typedef SmallVector<Decl *, 32> DeclPtrSmallVector;

class Algorithm
{
protected:
	static int32_t instCounter;
	int32_t uid;
	ResourceManager& resMgr;
	CompilerInstance& compInst;

	static int NamedCounter;
	static int VarCounter;
	static int TagCounter;
	static int LabelCounter;

public:
	virtual bool execute();

	Algorithm(ResourceManager& RM) : resMgr(RM), compInst(RM.getCompilerInstance()),
		uid(++instCounter) { }

	virtual ~Algorithm() { }

	int32_t getUid() {
		return uid;
	}

protected:
	StmtPtrSmallVector *ICCopy(Stmt *S);

	NullStmt *AddNewNullStmt();

	LabelStmt *AddNewLabel(Stmt *stBody);

	GotoStmt *AddNewGoto(LabelStmt *lblDes);

	bool DeallocateStmt(Stmt *S) {
		resMgr.getCompilerInstance().getASTContext().Deallocate(S);
	}
	
	bool renameVarDecl(NamedDecl *D);

	bool renameTagDecl(NamedDecl *D);

	bool renameNamedDecl(NamedDecl *D);

	IdentifierInfo& getUniqueVarName();

	IdentifierInfo& getUniqueTagName();

	IdentifierInfo& getUniqueLabelName();

	//create "OP(E)"
	Expr *BuildUnaryOperator(Expr *E, UnaryOperatorKind OP);

	//create "LHS op RHS"
	Expr *BuildBinaryOperator(Expr *LHS, Expr *RHS, BinaryOperatorKind OP);

	//create stmt: "lExpr = rExpr"
	Expr *BuildVarAssignExpr(VarDecl *VD, Expr *ER);

	//create "LHS = RHS"
	Expr *BuildCommonAssignExpr(Expr *LHS, Expr *RHS);

	BinaryOperator *BuildCommaExpr(Expr *EL, Expr *ER);

	//create "(E)"
	ParenExpr *BuildParenExpr(Expr *E);

	//build DeclRefExpr using a VarDecl
	DeclRefExpr *BuildVarDeclRefExpr(VarDecl *VD);

	ImplicitCastExpr *BuildImpCastExprToType(Expr *E, QualType Ty, CastKind CK);

	//Build var/tag decl which will be placed at the begnning of a function body
	DeclStmt *BuildDeclStmt(Decl *D);

	// Build ObjectType(Expr)
	// This is used to make expr: "ObjectType xx = ObjectType(Expr)"
	// E can be NULL, when "ObjctType xx = ObjectType()"
	// Ty must be a desugaredType
	Expr* BuildTempObjectConstuctExpr(QualType Ty, Expr *E);

	//build EL == ER
	Expr *BuildEqualCondExpr(Expr *EL, Expr *ER);

	//build EL <= EV or EV <= ER 
	BinaryOperator *BuildRangeCondExpr(Expr *EV, Expr *EL, Expr *EH);

	//build "Base[idx1][idx2]...[idxN]"
	Expr *BuildArraySubscriptExpr(Expr *Base, Expr **IdxList, unsigned int IdxNum);

	IdentifierInfo& getUniqueIdentifier(string sname, int &ccnt);

	IntegerLiteral *CreateIntegerLiteralX(int X);

	CXXBoolLiteralExpr *BuildCXXBoolLiteralExpr(bool Val);

	//const_cast<Ty>(E)
	CXXConstCastExpr *BuildCXXConstCastExpr(Expr *E, QualType Ty) {
		Sema& Sm = resMgr.getCompilerInstance().getSema();

		TypeSourceInfo *DI = dyn_cast<LocInfoType>(Ty)->getTypeSourceInfo();
		ExprResult ER = Sm.BuildCXXNamedCast(SourceLocation(), tok::kw_const_cast, DI, E,
			SourceLocation(), SourceLocation());
		assert(!ER.isInvalid());
		return dyn_cast<CXXConstCastExpr>(ER.get());
	}

	//create a new BuiltinType var
	DeclStmt* CreateVar(QualType Ty, DeclContext *DC = NULL, Expr *initList = NULL, clang::StorageClass SC = clang::SC_None);
	//
	//Create a new int var
	DeclStmt* CreateIntVar(DeclContext *DC = NULL, Expr *initVal = NULL, clang::StorageClass SC = clang::SC_None);

	//create a new bool var
	DeclStmt* CreateBoolVar(DeclContext *DC = NULL, Expr *initVal = NULL, clang::StorageClass SC = clang::SC_None);

	//auto remove NULL(not NullStmt)
	CompoundStmt* StVecToCompound(StmtPtrSmallVector *v);

	CompoundStmt* StmtToCompound(Stmt* S);

	//remove NULL and NullStmt in V
	//if succeeded, return V
	StmtPtrSmallVector* RemoveNullStmtInVector(StmtPtrSmallVector *V);

	//remove NULL and NullStmt children of S
	//if succeeded, return S
	CompoundStmt*  RemoveNullChildren(CompoundStmt *S);

	bool replaceChild(Stmt *Parent, Stmt *OldChild, Stmt *NewChild);

	// Will NOT automatically remove NULL or NullStmt in fpv
	bool updateChildrenStmts(Stmt* fparent, StmtPtrSmallVector *fpv);

	Stmt* NullChildStmt(Stmt *Parent);
};