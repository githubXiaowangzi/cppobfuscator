#include "SimplePrinter.h"
#include "SimplePrinterConsumer.h"

using namespace clang;

bool SimplePrinter::execute()
{
	DPRINT("alg started.");

	TranslationUnitDecl *tud = compInst.getASTContext().getTranslationUnitDecl();
	SimplePrinterConsumer consumer(llvm::errs(), &compInst);

	for (TranslationUnitDecl::decl_iterator I = tud->decls_begin(), E = tud->decls_end;
		I != E; I++) {
		SourceLocation Loc = I->getLocation();
		if (Loc.isInvalid() || compInst.getSourceManager().isInSystemHeader(Loc))
			continue;
		consumer.HandleTopLevelDecl(DeclGroupRef(*I));
	}
	DPRINT("alg finished.");
	return true;
}
