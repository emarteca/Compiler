#ifndef PARSER_H

	//void call();  // temp

	void module();
	void ImportList();
	void import();
	void DeclSeq();
	void ConstDecl();
	void identdef();
	void ConstExpr();
	void TypeDecl();
	void StrucType();
	void ArrayType();
	void length();
	void RecType();
	void BaseType();
	void qualident();
	void FieldListSeq();
	void FieldList();
	void IdentList();
	void PointerType();
	void ProcType();
	void VarDecl();
	void type();
	void ProcDecl();
	void ProcHead();
	void ProcBody();
	void FormParams();
	void FormParamSect();
	void FormType();
	void ForwardDecl();
	void Receiver();
	void StatSeq();
	void stat();
	void AssignStat();
	void ProcCall();
	void IfStat();
	void CaseStat();
	void Case();
	void CaseLabList();
	void LabelRange();
	void WhileStat();
	void RepeatStat();
	void ForStat();
	void LoopStat();
	void ExitStat();
	void WithStat();
	void guard();
	void expr();
	void SimplExpr();
	void factor();
	void term();
	void set();
	void elem();
	void designator();
	void selector();
	void ActParams();
	void ExprList();
	void ScaleFac();


#endif 