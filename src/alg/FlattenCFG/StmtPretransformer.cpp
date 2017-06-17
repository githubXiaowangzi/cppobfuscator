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
		declStack.pop_back();
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

bool StmtPretransformer::ExtractIfCondVarDecl(IfStmt *S) {
	if (!S) {
		return false;
	}

	ASTContext& Ctx = resMgr.getCompilerInstance().getASTContext();
	if (DeclStmt *stIfDcl = const_cast<DeclStmt *>(S->getConditionVariableDeclStmt())) {
		Stmt *Parent = parMap->getParent(S);
		assert(Parent && "IfStmt should have a parent");
		Expr *stDeclRef = BuildVarDeclRefExpr(S->getConditionVariable());
		Expr *newIfCond = BuildImpCastExprToType(stDeclRef, Ctx.BoolTy, clang::CK_LValueToRValue);
		S->setConditionVariable(Ctx, nullptr);
		S->setCond(newIfCond);

		StmtPtrSmallVector *compBody = new StmtPtrSmallVector();
		compBody->push_back(stIfDcl);
		compBody->push_back(S);
		CompoundStmt *newStmt = StVecToCompound(compBody);
		delete compBody;

		replaceChild(Parent, S, newStmt);
		parMap->addStmt(Parent);
	}
	return true;
}

bool StmtPretransformer::WhileToIf(Stmt *S) {
	DPRINT("while to if trans");
	ASTContext& Ctx = resMgr.getCompilerInstance().getASTContext();
	WhileStmt *whileSt = dyn_cast<WhileStmt>(S);

	//new LABELs
	LabelStmt *lblBegin = this->AddNewLabel(nullptr);
	LabelStmt *lblContinue = this->AddNewLabel(nullptr);
	LabelStmt *lblEnd = this->AddNewLabel(nullptr);

	//convert contiue and break in subtree
	this->InnerJumpToGoto(S, lblContinue, lblEnd);

	GotoStmt *gotoOfLblContinue = this->AddNewGoto(lblBegin);
	lblContinue->setSubStmt(gotoOfLblContinue);

	VarDecl *varDecl = whileSt->getConditionVariable();
	DeclStmt *oldDclSt = const_cast<DeclStmt*>(whileSt->getConditionVariableDeclStmt());
	Expr *condSt = whileSt->getCond();
	Stmt *wBody = whileSt->getBody();
	StmtPtrSmallVector *newIfBody = new StmtPtrSmallVector();
	if (wBody)
		newIfBody->push_back(wBody);
	newIfBody->push_back(lblContinue);
	Stmt *stNewIfBody = this->StVecToCompound(newIfBody);
	IfStmt *ifSt = new (Ctx)IfStmt(Ctx, SourceLocation(), false, nullptr, nullptr, condSt, stNewIfBody);

	StmtPtrSmallVector *lblBeginBody = new StmtPtrSmallVector();
	if (oldDclSt) {
		lblBeginBody->push_back(oldDclSt);
	}
	lblBeginBody->push_back(ifSt);
	lblBeginBody->push_back(lblEnd);
	Stmt *stLblBeginBody = this->StVecToCompound(lblBeginBody);
	lblBegin->setSubStmt(stLblBeginBody);

	Stmt *Parent = this->parMap->getParent(S);
	replaceChild(Parent, S, lblBegin);
	this->parMap->addStmt(S);

	delete newIfBody;
	delete lblBeginBody;

	return true;
}

bool StmtPretransformer::DoToIf(Stmt *S) {
	DPRINT("do to if trans");
	ASTContext &Ctx = this->resMgr.getCompilerInstance().getASTContext();
	DoStmt *DS = dyn_cast<DoStmt>(S);

	LabelStmt *lblBegin = this->AddNewLabel(nullptr);
	LabelStmt *lblDo = this->AddNewLabel(nullptr);
	LabelStmt *lblContinue = this->AddNewLabel(nullptr);
	LabelStmt *lblEnd = this->AddNewLabel(nullptr);

	this->InnerJumpToGoto(S, lblContinue, lblEnd);

	Stmt *oldDoBody = DS->getBody();
	if (oldDoBody == nullptr)
		oldDoBody = this->AddNewNullStmt();
	lblDo->setSubStmt(oldDoBody);

	GotoStmt *gotoOfLblContinue = this->AddNewGoto(lblBegin);
	Expr *whileCond = DS->getCond();
	IfStmt *ifOfLblContinue = new (Ctx)IfStmt(Ctx, SourceLocation(), false, nullptr, nullptr, whileCond, gotoOfLblContinue);
	lblContinue->setSubStmt(ifOfLblContinue);

	StmtPtrSmallVector *lblBeginBody = new StmtPtrSmallVector(); //the outer most do label stmt
	lblBeginBody->push_back(lblDo);
	lblBeginBody->push_back(lblContinue);
	lblBeginBody->push_back(lblEnd);
	lblBegin->setSubStmt(this->StVecToCompound(lblBeginBody));

	replaceChild(this->parMap->getParent(S), S, lblBegin);
	this->parMap->addStmt(S);

	delete lblBeginBody;

	return true;
}

bool StmtPretransformer::ForToIf(Stmt *S) {
	DPRINT("for to if trans");
	ASTContext &Ctx = this->resMgr.getCompilerInstance().getASTContext();
	ForStmt *FS = dyn_cast<ForStmt>(S);

	//outer most LABEL_FOR
	LabelStmt *lblFor = this->AddNewLabel(nullptr);
	LabelStmt *lblBegin = this->AddNewLabel(nullptr);
	LabelStmt *lblContinue = this->AddNewLabel(nullptr);
	LabelStmt *lblEnd = this->AddNewLabel(nullptr);

	//convert contiue and break in subtree
	this->InnerJumpToGoto(S, lblContinue, lblEnd);

	//LABEL_CONTINUE:
	StmtPtrSmallVector *lblContinueBody = new StmtPtrSmallVector();
	Stmt *oldForInc = FS->getInc();
	if (oldForInc)
		lblContinueBody->push_back(oldForInc);
	GotoStmt *jumpToLblBegin = this->AddNewGoto(lblBegin);
	lblContinueBody->push_back(jumpToLblBegin);
	//fill LABEL_CONTINUE:
	Stmt *stLblContinueBody = this->StVecToCompound(lblContinueBody);
	lblContinue->setSubStmt(stLblContinueBody);

	//new If stmt
	StmtPtrSmallVector *newIfBody = new StmtPtrSmallVector(); //
	Stmt *oldForBody = FS->getBody();
	if (oldForBody)
		newIfBody->push_back(oldForBody);
	newIfBody->push_back(lblContinue);
	Expr *oldForCond = FS->getCond();
	//if cond is null, create a "true" expr
	if (oldForCond == nullptr) {
		oldForCond = this->BuildCXXBoolLiteralExpr(true);
	}
	VarDecl *oldVarDcl = FS->getConditionVariable();
	DeclStmt *oldDclSt = const_cast<DeclStmt*>(FS->getConditionVariableDeclStmt());
	Stmt *stNewIfBody = this->StVecToCompound(newIfBody);
	IfStmt *newIfSt = new (Ctx)IfStmt(Ctx, SourceLocation(), false, nullptr, nullptr, oldForCond, this->StmtToCompound(stNewIfBody));

	//fill LABEL_BEGIN
	StmtPtrSmallVector lblBeginBody;
	if (oldDclSt) {
		lblBeginBody.push_back(oldDclSt);
	}
	lblBeginBody.push_back(newIfSt);
	lblBegin->setSubStmt(StVecToCompound(&lblBeginBody));

	//fill LABEL_FOR
	StmtPtrSmallVector *lblForBody = new StmtPtrSmallVector(); //
	lblForBody->push_back(FS->getInit());
	lblForBody->push_back(lblBegin);
	lblForBody->push_back(lblEnd);
	Stmt *stLblForBody = this->StVecToCompound(lblForBody);
	lblFor->setSubStmt(stLblForBody);
	//this->updateChildrenInEdge(stLblForBody);

	//modify old ForStmt
	replaceChild(this->parMap->getParent(S), S, lblFor);
	this->parMap->addStmt(S);

	delete lblContinueBody;
	delete newIfBody;
	delete lblForBody;

	return true;
}

bool StmtPretransformer::SwitchToIf(Stmt *S) {
	DPRINT("switch to if trans");
	ASTContext &Ctx = resMgr.getCompilerInstance().getASTContext();
	SwitchStmt *SS = dyn_cast<SwitchStmt>(S);

	//LABEL_SWITCH:
	DPRINT("LABEL_SWITCH");
	LabelStmt *stLblSwitch = AddNewLabel(nullptr); //to be filled

												//if no ConditionalVariable, create one
												// switch(a){...}  to   int t=(a); switch(t){..}
	if (!SS->getConditionVariable()) {
		DPRINT("create cond var");
		Decl *D = declStack.back();
		Expr *exprCond = SS->getCond();
		DeclStmt *dclSt = CreateIntVar(
			nullptr,
			BuildParenExpr(exprCond));
		VarDecl *varDcl = dyn_cast<VarDecl>(dclSt->getSingleDecl());
		SS->setConditionVariable(Ctx, varDcl);
		SS->setCond(BuildImpCastExprToType(exprCond, varDcl->getType(), clang::CK_LValueToRValue));
	}

	DPRINT("get switch sub ptr");
	Expr *EC = SS->getCond();
	DeclStmt *DS = const_cast<DeclStmt*>(SS->getConditionVariableDeclStmt());
	VarDecl *VD = SS->getConditionVariable();

	DPRINT("create if ");
	//IfStmt go switch goto
	IfStmt *stSwIf = new (Ctx) IfStmt(Ctx, SourceLocation(), false, nullptr, nullptr, nullptr, nullptr);

	//switch body
	Stmt *stSwBody = SS->getBody();

	//LABEL_BREAK:
	LabelStmt *stLblBreak = AddNewLabel(nullptr);

	DPRINT("iterator case/default");
	//process all cases
	//record default and add it last
	LabelStmt *stLblDefault = nullptr;
	IfStmt *lastIf = nullptr;
	for (SwitchCase *SC = SS->getSwitchCaseList();
		SC; SC = SC->getNextSwitchCase()) {
		LabelStmt *stLblCase = AddNewLabel(nullptr);

		//record the replacing stmt of CaseStmt
		StmtNodeMap::iterator it = this->stmtMap.find(SC);
		assert(it != this->stmtMap.end() && it->second && "stmt dfs info not found");
		it->second->stNew = stLblCase;
		DPRINT("record case: (%x) %x %x %x", it->second, it->second->stOrig, it->second->pInEdge, it->second->stNew);

		if (isa<CaseStmt>(*SC)) {
			DPRINT("case stmt reached %x", SC);
			CaseStmt *stCase = dyn_cast<CaseStmt>(SC);

			//add if goto
			Expr *expL = stCase->getLHS();
			Expr *expR = stCase->getRHS();
			Expr *expIfCond = expR ?
				BuildRangeCondExpr(BuildVarDeclRefExpr(VD), expL, expR)
				: BuildEqualCondExpr(BuildVarDeclRefExpr(VD), expL);

			//goto LABEL_CASE
			GotoStmt *stGoto = AddNewGoto(stLblCase);
			DPRINT("set else if");

			if (lastIf) {
				IfStmt *elseIf = new (Ctx)
					IfStmt(Ctx, SourceLocation(), false, nullptr, nullptr, expIfCond, stGoto);
				lastIf->setElse(elseIf);
				lastIf = elseIf;
			}
			else {
				stSwIf->setCond(expIfCond);
				stSwIf->setThen(stGoto);
				lastIf = stSwIf;
			}
			DPRINT("case stmt end");
		}
		else if (isa<DefaultStmt>(*SC)) {
			DPRINT("default stmt reached %x", SC);
			stLblDefault = stLblCase;
			DPRINT("default stmt end");
		}
		else {
			assert(false && "SwitchCase type unknown");
		}
	}
	//add the last else goto
	DPRINT("add last else goto");
	GotoStmt *lastGoto = stLblDefault ?
		AddNewGoto(stLblDefault)
		: AddNewGoto(stLblBreak);
	if (lastIf) {
		lastIf->setElse(lastGoto);
	}
	else {
		stSwIf->setCond(BuildCXXBoolLiteralExpr(true)); // if(true)
		stSwIf->setThen(lastGoto);
	}

	DPRINT("transform break");

	//preserve ContinueStmt
	for (int I = this->stmtStack.size() - 1;
		I >= 0 && this->stmtStack[I]->stOrig != S;
		--I) {
		//Child is the orignal child, not nessessarily equal to *pChild
		StmtPretransInfo *node = this->stmtStack[I];
		Stmt *Child = node->stOrig, **pChild = node->pInEdge, *Replace = node->stNew, *Parent = this->parMap->getParent(Child);

		//preserve ContinueStmt
		if (isa<ContinueStmt>(Child)) {
			continue;
		}
		DPRINT("pop stmt %s (%x) %x %x %x", Child->getStmtClassName(), &node, Child, pChild, Replace);

		//transform break
		if (isa<BreakStmt>(Child)) {
			DPRINT("break reached");
			GotoStmt *jumpToLblBreak = this->AddNewGoto(stLblBreak);
			//*pChild = jumpToLblBreak; //FIXME memory leak 
			replaceChild(Parent, Child, jumpToLblBreak);
			DPRINT("break end");
		}
		else if (isa<SwitchCase>(Child)) {
			DPRINT("SwitchCase reached %x", Child);
			//modify old case ptr
			SwitchCase *swCas = dyn_cast<SwitchCase>(Child);
			LabelStmt *lblSt = dyn_cast<LabelStmt>(Replace);
			lblSt->setSubStmt(swCas->getSubStmt());
			//*pChild = Replace; //FIXME memory leak
			replaceChild(Parent, Child, Replace);
			DPRINT("switchCase ended");
		}

		delete node;
		this->stmtStack.erase(this->stmtStack.begin() + I);
		this->stmtMap.erase(Child);
	}

	DPRINT("set new switch body");

	StmtPtrSmallVector *lblSwitchBody = new StmtPtrSmallVector();
	//T t=x;
	if (DS)
		lblSwitchBody->push_back(DS);
	lblSwitchBody->push_back(stSwIf);
	if (stSwBody)
		lblSwitchBody->push_back(stSwBody);
	lblSwitchBody->push_back(stLblBreak);
	Stmt *stLblSwitchBody = this->StVecToCompound(lblSwitchBody);

	//fill LABEL_SWITCH body
	stLblSwitch->setSubStmt(stLblSwitchBody);
	//this->updateChildrenInEdge(stLblSwitchBody);
	//modify parent ptr
	//S = stLblSwitch;
	replaceChild(this->parMap->getParent(S), S, stLblSwitch);
	this->parMap->addStmt(S);

	DPRINT("done");

	delete lblSwitchBody;
	DPRINT("free memory");

	return true;
}

bool StmtPretransformer::InnerJumpToGoto(const Stmt *stRoot, LabelStmt *stLblContinue, LabelStmt *stLblBreak) {
	ASTContext &Ctx = resMgr.getCompilerInstance().getASTContext();
	//convert contiue and break in subtree
	for (int I = this->stmtStack.size() - 1;
		I >= 0 && this->stmtStack[I]->stOrig != stRoot;
		--I) {
		StmtPretransInfo *node = this->stmtStack[I];
		Stmt *Child = node->stOrig, **pChild = node->pInEdge, *Replace = node->stNew, *Parent = this->parMap->getParent(Child);
		//preserve DefaultStmt/CastStmt
		if (isa<SwitchCase>(Child)) {
			continue;
		}
		DPRINT("pop stmt %s (%x) %x %x %x", Child->getStmtClassName(), &node, Child, pChild, Replace);

		if (isa<ContinueStmt>(Child)) {
			if (stLblContinue) {
				GotoStmt *jumpToLblContinue = this->AddNewGoto(stLblContinue);
				replaceChild(Parent, Child, jumpToLblContinue);
				//*pChild = jumpToLblContinue; //FIXME: memory leak
			}
		}
		else if (isa<BreakStmt>(Child)) {
			if (stLblBreak) {
				GotoStmt *jumpToLblEnd = this->AddNewGoto(stLblBreak);
				replaceChild(Parent, Child, jumpToLblEnd);
				//*pChild = jumpToLblEnd; //FIXME: memory leak
			}
		}

		delete node;
		this->stmtStack.erase(this->stmtStack.begin() + I);
		this->stmtMap.erase(Child);
	}

	return true;
}

bool StmtPretransformer::updateChildrenInEdge(Stmt *S) {
	if (!S) {
		DPRINT("Stmt NULL");
		return true;
	}
	//FIXME: inefficient, replace with a new data structure
	this->parMap->addStmt(S);

	return true;
}