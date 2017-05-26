#include "ResourceManager.h"
#include "HeaderSearchPath.h"
#include "alg/InitParseConsumer.h"

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

using namespace clang;
using llvm::DenseSet;

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
		hsOpts.AddPath(HS_PATHS[i], frontend::CXXSystem, false, false);
	}
	//compInvo.setLangDefaults(compInst->getLangOpts(), IK_CXX, , LangStandard::lang_cxx98);
	compInst->createDiagnostics(nullptr, false);

	std::shared_ptr<TargetOptions> tarOpts = std::make_shared<TargetOptions>();
	tarOpts->Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *tarInfo = TargetInfo::CreateTargetInfo(compInst->getDiagnostics(), tarOpts);
	compInst->setTarget(tarInfo);

	compInst->createFileManager();
	compInst->createSourceManager(compInst->getFileManager());
	compInst->createPreprocessor(clang::TU_Complete);
	compInst->createASTContext();
	compInst->setASTConsumer(std::make_unique<InitParseConsumer>(this->decls, this->compInst.get()));
	Preprocessor& PP = compInst->getPreprocessor();
	PP.getBuiltinInfo().initializeBuiltins(PP.getIdentifierTable(), PP.getLangOpts());

	compInst->createSema(clang::TU_Complete, nullptr);

	rw.reset(new Rewriter());
	rw->setSourceMgr(compInst->getSourceManager(), compInst->getLangOpts());
}

bool ResourceManager::initParseAST() {
	assert(optTable->getOption(options::Argument).size() == 1 && "No src file specified.");

	string srcFileFullName = optTable->getOption(options::Argument).front();
	FileManager& fileMgr = compInst->getFileManager();
	SourceManager& srcMgr = compInst->getSourceManager();
	const FileEntry *fileIn = fileMgr.getFile(srcFileFullName);
	srcMgr.setMainFileID(srcMgr.createFileID(fileIn, clang::SourceLocation(), clang::SrcMgr::C_User));
	compInst->getDiagnosticClient().BeginSourceFile(compInst->getLangOpts(), &compInst->getPreprocessor());

	ParseAST(compInst->getSema());
	return true;
}

void ResourceManager::rewriteToFile() {
	SourceManager& srcMgr = compInst->getSourceManager();
	DenseSet<FileID> q;
	TranslationUnitDecl *decls = compInst->getASTContext().getTranslationUnitDecl();
	for (TranslationUnitDecl::decl_iterator I = decls->decls_begin(), E = decls->decls_end();
		I != E; I++) {
		Decl *D = *I;
		SourceLocation Loc = D->getLocation();
		if (Loc.isInvalid() || srcMgr.isInSystemHeader(Loc)) {
			continue;
		}
		FileID thisFileID = srcMgr.getFileID(Loc);
		q.insert(thisFileID);
	}

	const RewriteBuffer *rwBuf = nullptr;
	std::error_code errInfo;
	OptionTable::OptValueListTy& baseOutDir = optTable->getOption(options::OPT_Directory);
	for (DenseSet<FileID>::Iterator I = q.begin(), E = q.end();
		I != E; I++) {
		FileID thisFileID = *I;
		rwBuf = rw->getRewriteBufferFor(thisFileID);
		string thisFileName = srcMgr.getFileEntryForID(thisFileID)->getName();
		long fileNamePos = thisFileName.find_last_of("/\\") + 1;
		//windows下，需要把盘符和:去掉
		long driveNamePos = thisFileName.find_first_of(":") + 1;
		string pathName = thisFileName.substr(driveNamePos == std::string::npos ? 0 : driveNamePos, fileNamePos);
		string fileName = thisFileName.substr(fileNamePos, thisFileName.length());
		if (baseOutDir.empty()) {
			thisFileName = pathName + "_._" + fileName;
		}
		else {
			if (pathName.at(0) == '/' || pathName.substr(0, 2) == "\\")
				pathName = baseOutDir.front() + pathName;
			else
				pathName = baseOutDir.front() + "/" + pathName;
			char createPath[512];
			sprintf(createPath, "md \"%s\"", pathName.c_str());
			system(createPath);
			thisFileName = pathName + fileName;
		}
		if (rwBuf != nullptr) {
			llvm::raw_fd_ostream fos(thisFileName, errInfo, llvm::sys::fs::F_RW);
			fos << string(rwBuf->begin(), rwBuf->end());
			fos.close();
			DPRINT("src %s rewrite.", thisFileName.c_str());
		}
		else {
			DPRINT("src %s no changed.", thisFileName.c_str());
		}
	}
}

bool ResourceManager::prettyPrint(/*llvm::raw_ostream &out*/) {
	ASTContext& Ctx = compInst->getASTContext();
	SourceManager& srcMgr = compInst->getSourceManager();
	PrintingPolicy policy = compInst->getASTContext().getPrintingPolicy();
	NullStmt *nullSt = new (compInst->getASTContext())NullStmt(SourceLocation());

	DenseSet<FileID> createFileID;
	FileID lastFileID;
	std::unique_ptr<llvm::raw_fd_ostream> fout;
	std::error_code errInfo;
	TranslationUnitDecl *decls = compInst->getASTContext().getTranslationUnitDecl();
	for (TranslationUnitDecl::decl_iterator I = decls->decls_begin(), E = decls->decls_end();
		I != E; I++) {
		Decl *D = *I;
		SourceLocation Loc = D->getLocation();
		if (Loc.isInvalid() || srcMgr.isInSystemHeader(Loc))
			continue;
		FileID thisFileID = srcMgr.getFileID(Loc);
		if (thisFileID != lastFileID) {
			if (fout) {
				fout.get()->close();
				fout.reset();
			}
			string thisFileName = srcMgr.getFileEntryForID(thisFileID)->getName();
			thisFileName.insert(thisFileName.find_last_of("/\\") + 1, "@");
			fout.reset(new llvm::raw_fd_ostream(thisFileName, errInfo, llvm::sys::fs::F_Append));
			lastFileID = thisFileID;
			createFileID.insert(thisFileID);
			DPRINT("Open desfile %s", thisFileName.c_str());
		}

		D->print(*fout.get(), policy);
		nullSt->printPretty(*fout.get(), nullptr, policy);
	}
	if (fout) {
		fout->close();
	}
	return true;
}