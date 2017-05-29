#include "VarRenamer.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace std;
using namespace clang;

bool VarRenamer::HandleDecl(Decl *D) {
	//only rename vars in function, so far
	assert(isa<FunctionDecl>(D) && "not a function decl");
	DPRINT("START VarRenamer");
	this->TraverseDecl(D);
	DPRINT("END VarRenamer");
	return true;
}

bool VarRenamer::VisitDecl(Decl *&D) {
	if (!D) {
		return true;
	}

	DPRINT("decl: %s", D->getDeclKindName());

	Rewriter &rw = resMgr.getRewriter();
	if (VarDecl *VD = dyn_cast<VarDecl>(D)) {
		DPRINT(" ---- name = %s | type = %s | desugared type = %s | const = %d | extern = %d | POD = %d",
			VD->getQualifiedNameAsString().c_str(),
			VD->getType().getAsString().c_str(),
			VD->getType().getDesugaredType(this->resMgr.getCompilerInstance().getASTContext()).getAsString().c_str(),
			VD->isConstexpr(),
			VD->hasExternalStorage(),
			VD->getType().isPODType(this->resMgr.getCompilerInstance().getASTContext()));
		if (VD->hasInit()) {
			VD->getInit()->dumpPretty(resMgr.getCompilerInstance().getASTContext());
		}

		QualType Ty = VD->getType();
		if ((VD->isLocalVarDecl() || isa<ParmVarDecl>(VD)) && !VD->hasExternalStorage()) {
			this->renameVarDecl(VD);
			if (isa<ParmVarDecl>(VD)) {
				rw.ReplaceText(SourceRange(VD->getLocation()), VD->getName());
			}
		}
	}
	else if (isa<TagDecl>(D)) {
		TagDecl *TD = dyn_cast<TagDecl>(D);
		this->renameTagDecl(TD);
	}
	else if (isa<EnumConstantDecl>(D)) {
		EnumConstantDecl *ECD = dyn_cast<EnumConstantDecl>(D);
		this->renameVarDecl(ECD);
	}
	return true;
}