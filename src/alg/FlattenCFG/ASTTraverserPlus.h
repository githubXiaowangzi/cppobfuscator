#pragma once

#include "../../stdafx.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

template <typename Derived>
class ASTTraverserPlus : public RecursiveASTVisitor<ASTTraverserPlus<Derived>>
{
public:
	typedef ASTTraverserPlus<Derived> thisType;
	Derived &getDerived() {
		return *static_cast<Derived *>(this);
	}

	bool TraverStmt(Stmt *s);

	bool WalkupFromStmt(Stmt *s) {
		return getDerived().VistiStmt(s);
	}
	bool VisitStmt(Stmt *s) {
		return true;
	}
	bool ExitStmt(Stmt *S) { 
		return true; 
	}

	bool TraverseDecl(Decl *D);

	bool WalkUpFromDecl(Decl *D) { 
		return getDerived().VisitDecl(D); 
	}

	bool VisitDecl(Decl *D) { 
		return true; 
	}
	bool ExitDecl(Decl *D) { 
		return true;
	}

	// Traverse all decls in TranslationUnitDecl.
	// If SM is not NULL, then we'll  use it to help skip system headers
	bool TraverseTranslationUnitDecl(TranslationUnitDecl *TUD, SourceManager *SM = nullptr);
	bool BeforeTraverseDecl(Decl *D) { 
		return true; 
	}
	bool AfterTraverseDecl(Decl *D) { 
		return true; 
	}
};

template<typename Derived>
bool ASTTraverserPlus<Derived>::TraverseStmt(Stmt *S) {
	if (!S) {
		return true;
	}
	DPRINT("enter stmt %x (%s) | range ", (unsigned int)S, S->getStmtClassName());
	RecursiveASTVisitor<ThisType>::TraverseStmt(S);
	DPRINT("exit stmt %x (%s)", (unsigned int)S, S->getStmtClassName());
	getDerived().ExitStmt(S);
	return true;
}

template<typename Derived>
bool ASTTraverserPlus<Derived>::TraverseDecl(Decl *D) {
	if (!D) {
		return true;
	}
	DPRINT("enter decl %x (%s)", (unsigned int)D, D->getDeclKindName());
	RecursiveASTVisitor<ThisType>::TraverseDecl(D);
	DPRINT("exit decl %x (%s)", (unsigned int)D, D->getDeclKindName());
	getDerived().ExitDecl(D);
	return true;
}

template<typename Derived>
bool ASTTraverserPlus<Derived>::TraverseTranslationUnitDecl(TranslationUnitDecl *TUD, SourceManager *SM) {
	if (!TUD) {
		return true;
	}
	for (TranslationUnitDecl::decl_iterator I = TUD->decls_begin(), IEnd = TUD->decls_end();
		I != IEnd; ++I) {
		Decl *D = *I;
		if (SM && SM->isInSystemHeader(D->getLocation())) {
			continue;
		}
		if (!getDerived().BeforeTraverseDecl(D)) {
			return false;
		}
		if (!getDerived().TraverseDecl(D)) {
			return false;
		}
		if (!getDerived().AfterTraverseDecl(D)) {
			return false;
		}
	}
	return true;
}