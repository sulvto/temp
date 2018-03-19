#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;



//===-------------------------------===
// Lexer
//===-------------------------------===

// The lexer returns tokens
enum Token {
	tok_eof = -1,

	// commands
	tok_def = -2,
	tok_extern = -3,

	// primary
	tok_identifier = -4,
	tok_number = -5
};

static std::string IdentifierStr;	// Filled in if tok_identifier
static double NumVal;				// Filled in if tok_number;

// gettok -Return the next token from standard input
static int gettok() {
	static int LastChar = ' ';

	// Skip any whitespace
	while (isspace(LastChar)) 
		LastChar = getchar();

	// identifier:[a-zA-Z][a-zA-Z0-9]*
	if (isalpha(LastChar)) {
		IdentifierStr = LastChar;
		while (isalnum((LastChar = getchar())))
			IdentifierStr += LastChar;
		if (IdentifierStr == "def")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		return tok_identifier;
	}

	// Number:[0-9.]+
	if (isdigit(LastChar) || LastChar == '.') {
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = getchar();
		} while(isdigit(LastChar) || LastChar == '.');
		
		NumVal = strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	if (LastChar == '#') {
		// Comment until end of line.
		do
			LastChar = getchar();
		while (LastChar != EOF && LastChar !='\n' && LastChar != '\r');

		if (LastChar != EOF) {
			return gettok();
		}
	}

	// Check for end of file. Don`t eat the EOF
	if (LastChar == EOF) 
		return tok_eof;

	// Otherwise, just return the charcter as ascii value.
	int ThisChar = LastChar;
	LastChar = getchar();
	return ThisChar;
}



//===-------------------------------===
// Abstract Syntax Tree
//===-------------------------------===

// ExprAST -Base class for all expression nodes.
class ExprAST {
public:
	virtual ~ExprAST() {};
	virtual Value *Codegen() = 0;
};

// NumberExprAST -Expression class for numberic literals like "1.0"
class NumberExprAST : public ExprAST {
	double Val;

public:
	NumberExprAST(double Val): Val(Val) {}
	Value *Codegen();
};

// VariableExprAST -Expression class for referencing a variable,like "a".
class VariableExprAST : public ExprAST {
	std::string Name;

public:
	VariableExprAST(const std::string &Name) : Name(Name) {}
	Value *Codegen();
};

// BinaryExprAST -Expression class for a brinary operator.
class BinaryExprAST : public ExprAST {
	char Op;
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
					std::unique_ptr<ExprAST> RHS)
			: Op(Op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
	Value *Codegen();
};

// CallExprAST -Expression class for function calls.
class CallExprAST : public ExprAST {
	std::string Callee;
	std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(const std::string &Callee,
					std::vector<std::unique_ptr<ExprAST>> Args)
			: Callee(Callee), Args(std::move(Args)) {}
	Value *Codegen();
};

// PrototypeAST -This class repesents the "prototype" for a function,
// wgich captures its name, and its argument names (thus implicitly the number
// of arguments the function takes).
class PrototypeAST {
	std::string Name;
	std::vector<std::string> Args;

public:
	PrototypeAST(const std::string &Name, std::vector<std::string> Args) 
			:Name(Name), Args(std::move(Args)) {}

	Function *Codegen();
	const std::string &getName() const { return Name; }
};

// FunctionAST -This class repesents a function definition itself.
class FunctionAST {
	std::unique_ptr<PrototypeAST> Proto;
	std::unique_ptr<ExprAST> Body;

public:
	FunctionAST(std::unique_ptr<PrototypeAST> Proto,
					std::unique_ptr<ExprAST> Body)
			:Proto(std::move(Proto)), Body(std::move(Body)) {}
	Function *Codegen();
};



//===-------------------------------===
// Parser
//===-------------------------------===

static std::unique_ptr<ExprAST> ParseExpression();
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);

// CurTok/getNextToken -Provide a simple token buffer, CurTok is the current
// token the parser is looking at, getNextToken reads another token from the
// lexer and updates CurTok with its results.
static int CurTok;
static int getNextToken() {
	return CurTok = gettok();
}

// BincpPrecedence -This holds the precedence for each binary operator that is
// defined.
static std::map<char, int> BincpPrecedence;

// GetTokPrecedence -Get the precedence of the pending binary operator token.
static int GetTokPrecedence() {
	if (!isascii(CurTok)) 
		return -1;

	// Make sure it's a declaned bincp.
	int TokPrec = BincpPrecedence[CurTok];
	if (TokPrec <= 0) return -1;
	return TokPrec;
}

// LogError* -There are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
	fprintf(stderr, "LogError:%s\n", Str);
	return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
	LogError(Str);
	return nullptr;
}

// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
	auto Result = llvm::make_unique<NumberExprAST>(NumVal);
	getNextToken();	// consume the number
	return std::move(Result);
}

// parenexpr ::= '('expression')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
	getNextToken();	// eat (.
	auto V = ParseExpression();
	if (!V)
		return nullptr;
	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // eat ).
	return V;
}

// identifierexpr
// 		::= identifier
// 		::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken();	// eat identifier.
	if (CurTok != '(') 
		return llvm::make_unique<VariableExprAST>(IdName);
	
	// Call.
	getNextToken();	// eat (.
	std::vector<std::unique_ptr<ExprAST>> Args;
	if (CurTok != ')') {
		while (1) {
			if (auto Arg = ParseExpression())
				Args.push_back(std::move(Arg));
			else
					return nullptr;
			if (CurTok == ')')
					break;
			if (CurTok != ',') 
				return LogError("Ecpected ')' or ',' in argument list");
			getNextToken();
		}
	}

	getNextToken();	// eat ).

	return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

// primary
// 		::= identifier
// 		::= numberexpr
// 		::= parenexpr
static std::unique_ptr<ExprAST> ParsePrimay() {
	switch (CurTok) {
		default:
			return LogError("unknown token when expecting an expression");
		case tok_identifier:
			return ParseIdentifierExpr();
		case tok_number:
			return ParseNumberExpr();
		case '(':
			return ParseParenExpr();
	}
}

// expression
// 		::= primarybinoprhs
//
static std::unique_ptr<ExprAST> ParseExpression() {
	auto LHS = ParsePrimay();
	if (!LHS) return nullptr;

	return ParseBinOpRHS(0, std::move(LHS));
}

// binoprhs
// 		::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
	// If this is binop, find its precedence.
	while (1) {
		int TokPrec = GetTokPrecedence();
		// If this is a binop that binds at least as tightly as the current binop.
		// consume it, otherwise we are clone.
		if (TokPrec < ExprPrec)
				return LHS;

		// this is binop.
		int BinOp = CurTok;
		getNextToken();	// eat binop

		// Parse the primary expression after the binary operator.
		auto RHS = ParsePrimay();
		if (!RHS) return nullptr;

		// If BinOp binds tightly with RHS than the operator after RHS,let
		// the pending operator take RHS as its LHS.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if (!RHS) return nullptr;
		}

		// Merge LHS/RHS.
		LHS = llvm::make_unique<BinaryExprAST>(BinOp, std::move(LHS),
					   	std::move(RHS));
	}	// loop around to the top of the while loop.
}

// prototype
// 		::=id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
	if (CurTok != tok_identifier) 
		return LogErrorP("Expected function name in prototype");
	
	std::string FnName = IdentifierStr;
	getNextToken();

	if (CurTok != '(') 
		return LogErrorP("Expected '(' in prototype");

	// Read the list of argument names.
	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier) 
		ArgNames.push_back(IdentifierStr);
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// success.
	getNextToken();	// eat')

	return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
	getNextToken();	// eat def.
	auto Proto = ParsePrototype();
	if (!Proto) return nullptr;

	if (auto E = ParseExpression())
		return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));

	return nullptr;
}

// external := 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
	getNextToken();	// eat extern.
	return ParsePrototype();
}

// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
	if (auto E = ParseExpression()) {
		// Make an anonymous proto.
		auto Proto = llvm::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
		return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}

	return nullptr;
}



//===-------------------------------===
// Code Generation
//===-------------------------------===

static LLVMContext TheContext;
static Module *TheModule;
static IRBuilder<> Builder(TheContext);
static std::map<std::string, Value *> NamedValues;

Value *LogErrorV(const char *Str) { LogError(Str); return nullptr; }

Function *LogErrorF(const char *Str) { LogError(Str); return nullptr; }

Value *NumberExprAST::Codegen() {
	return ConstantFP::get(TheContext, APFloat(Val));
}

Value *VariableExprAST::Codegen() {
	// Lock this variable up in the function
	Value * V = NamedValues[Name];
	return V ? V : LogErrorV("Unknown variable name");
}

Value *BinaryExprAST::Codegen() {
	Value *L = LHS->Codegen();
	Value *R = RHS->Codegen();

	if (L == 0 || R == 0) return 0;

	switch (Op) {
		case '+': return Builder.CreateFAdd(L, R, "addtmp");
		case '-': return Builder.CreateFSub(L, R, "subtmp");
		case '*': return Builder.CreateFMul(L, R, "multmp");
		case '<': 
			L = Builder.CreateFCmpULT(L, R, "cmptmp");
			// Convert bool 0/1 to double 0.0 or 1.0
			return Builder.CreateUIToFP(L, Type::getDoubleTy(TheContext), "booltmp");
		default: return LogErrorV("invalid binary operator");
	}
}

Value *CallExprAST::Codegen() {
	// Lock up the name in the global module table.
	Function *CalleeF = TheModule->getFunction(Callee);
	if (CalleeF == 0) return LogErrorV("Unknown function referenced");
	
	// If argument mismatch error.
	if (CalleeF->arg_size() != Args.size()) 
		return LogErrorV("Incorrect # arguments passed");
	
	std::vector<Value*> ArgsV;
	for (unsigned i = 0,e = Args.size(); i != e; ++i) {
		ArgsV.push_back(Args[i]->Codegen());
		if (ArgsV.back() == 0) return 0;
	}

	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

Function *PrototypeAST::Codegen() {
	// Make the function type: double (double, doubel)etc.
	std::vector<Type*> Doubles(Args.size(),
						Type::getDoubleTy(TheContext));
	FunctionType *FT = FunctionType::get(Type::getDoubleTy(TheContext),
						Doubles, false);

	Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule);

	// If F conflicted, there was already someting named 'Name'. If it has a
	// body, dont't allow redefinition or reextern.
	if (F->getName() != Name) {
		// Delete the one we just made and get the existing one.
		F->eraseFromParent();
		F = TheModule->getFunction(Name);
	}

	// If F already has a body, reject this.
	if (!F->empty()) {
		return LogErrorF("redefinition of function");
	}

	// If F took a different number of args, reject.
	if (F->arg_size() != Args.size()) {
		LogErrorF("redefinition of function with ifferent # args");
	}

	// Set names for all arguments.
	unsigned Idx = 0;
	for (Function::arg_iterator AI = F->arg_begin(); Idx != Args.size(); 
					++AI, ++Idx) {
		AI->setName(Args[Idx]);

		// Add arguments to variable symbol table.
		NamedValues[Args[Idx]] = AI;
	}
	return F;
}

Function *FunctionAST::Codegen() {
	// First, check for an exitsting function from a previous 'extern' declaration.
	Function *TheFunction = TheModule->getFunction(Proto->getName());

	if (!TheFunction) TheFunction = Proto->Codegen();

	if (!TheFunction) return nullptr;

	if (!TheFunction->empty()) 
		return LogErrorF("Function cannot be redefined.");

	// Creat a new basic block to start insertion into.
	BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
	Builder.SetInsertPoint(BB);

	// Record the function arguments in the NamedValues map.
	NamedValues.clear();
	for (auto &Arg : TheFunction->args()) 
		NamedValues[Arg.getName()] = &Arg;

	if (Value *RetVal = Body->Codegen()) {
		// Finish off the function
		Builder.CreateRet(RetVal);

		// Validate the generated code, checking for consis tency.
		verifyFunction(*TheFunction);

		return TheFunction;
	}

	TheFunction->eraseFromParent();
	return nullptr;
}



//===-------------------------------===
// Top-Level parsing and JIT Driver
//===-------------------------------===

static void HandleDefinition() {
	if (auto FnAST = ParseDefinition()) {
		if (auto *FnIR = FnAST->Codegen()) {
			fprintf(stderr, "Read function definition:");
			FnIR->dump();
			fprintf(stderr, "\n");
		}
	} else {
		// Skip token for error recovery.
        getNextToken();
	}
}

static void HandleExtern() {
	if (auto ProtoAST = ParseExtern()) {
		if (auto *FnIR = ProtoAST->Codegen()) {
			fprintf(stderr, "Read extern:");
			FnIR->dump();
			fprintf(stderr, "\n");
		}
	} else {
		// Skip token for error recovery.
		getNextToken();
	}
}

static void HandleTopLevelExpression() {
	// Evaluate a tope-level expression into an anonymous function.
	if (auto FnAST = ParseTopLevelExpr()) {
			if (auto *FnIR = FnAST->Codegen()) {
				fprintf(stderr, "Read top-level expression:");
				FnIR->dump();
				fprintf(stderr, "\n");
			}
	} else {
		// Skip token for error recovery.
		getNextToken();
	}
}

// top ::= definition | extern | expression | ';'
static void MainLoop() {
	while (1) {
		fprintf(stderr, "ready> ");
		switch (CurTok) {
			case tok_eof:
				return;
			case ';':	// ignore top-level semicolons
				getNextToken();
				break;
			case tok_def:
				HandleDefinition();
				break;
			case tok_extern:
				HandleExtern();
				break;
			default:
				HandleTopLevelExpression();
				break;
		}
	}
}


// clang++ -g -O3 toy.cpp `llvm-config --cxxflags --ldflags --system-libs --libs core` -o toy 
int main() {
	// Install stanard binary operators.
	// 1 is lower precedence,
	BincpPrecedence['<'] = 10;
	BincpPrecedence['+'] = 20;
	BincpPrecedence['-'] = 20;
	BincpPrecedence['*'] = 40;	// highest.

	// Prime the first token.
	fprintf(stderr, "ready> ");
	getNextToken();

	// Make the module, which hilds all the code.
	TheModule = new Module("my cool jit", TheContext);

	// Run the main "interpreter loop" now.
	MainLoop();

	// Print out all of the generated code.
	TheModule->print(errs(), nullptr);

	return 0;
}
