#include "ForTransformer.h"
using namespace std;
using namespace clang;

bool
ForTransformer::HandleTopLevelDecl(DeclGroupRef D) {
	for(DeclGroupRef::iterator I = D.begin(), E = D.end();
			I != E; ++I ){
		Stmt *stmtBody = (*I)->getBody();
		if(stmtBody) {
			CompoundStmt *stmtBodyComp= dyn_cast<CompoundStmt>(stmtBody);
			LabelStmt *stmtLbl;
			SourceLocation numloc;
		}
	}
}

bool
ForTransformer::execute() {
	ForVisitor visitor(*this);
	DeclGroupRefVec &Ds = this->resMgr.getDeclGroupRefVec();
	for(int i = 0; i < Ds.size(); i++) {
		for(DeclGroupRef::iterator I = Ds[i].begin(), E = Ds[i].end();
				I != E; ++I) {
			Decl *d = *I;

			if(this->compInst.getSourceManager().isInSystemHeader(d->getLocation())){
				continue;
			}
			visitor.TraverseDecl(d);
		}
	}	
}

bool 
ForTransformer::ForVisitor::VisitStmt(Stmt *s) {
	DPRINT("stmt: %s", s->getStmtClassName());
	if(isa<ForStmt>(s)) {
		ForStmt *forSt = dyn_cast<ForStmt>(s);
		//NullStmt *nullSt = new (forTrans.compInst.getASTContext()) NullStmt(SourceLocation());
		BuiltinType *biInt = new BuiltinType(BuiltinType::Int);

		StmtPtrSmallVector *fbody = forTrans.ICCopy(forSt->getBody());
		Expr* incS = (forSt->getInc()!=NULL) ? forSt->getInc() : forTrans.CrLiteralX(1, biInt);
		LabelStmt *lsmt = forTrans.AddNewLabel(incS);
		fbody[0].insert(fbody[0].begin(), lsmt);
		forSt->setBody(forTrans.StVecToCompound(fbody));
		//TODO

		//forTrans.updateChildrenStmts(s, <#StmtPtrSmallVector *fpv#>)
	}

	return true;
}


