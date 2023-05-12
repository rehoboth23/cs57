/**
 * @file optimizer.c
 * @author Rehoboth Okorie
 * @brief optimizer with LLVM
 * constant folding
 * common sub-expression
 * dead code
 * constant propagation
 *
 * @version 0.1
 * @date 2023-05-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include "llvm/IR/IRBuilder.h"
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/Error.h>
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/BasicBlock.h"
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/raw_os_ostream.h>
#include "optimizer.h"

using namespace std;

void log(string s)
{
#ifdef LOG
	cout << s << endl;
#endif
}

string getInstructionString(llvm::Instruction &inst)
{
	string instStr;
	llvm::raw_string_ostream(instStr) << inst;
	return instStr;
}

void eliminateCommonSubExpression(llvm::BasicBlock &basicBlock, bool &change)
{
	map<pair<llvm::Value *, llvm::Value *>, pair<unsigned, llvm::Instruction *> *> commonSubexpressions;
	map<llvm::Value *, vector<pair<llvm::Value *, llvm::Value *>> *> opTrace;

	for (llvm::Instruction &inst : basicBlock)
	{
		unsigned opCode = inst.getOpcode();
		llvm::Value *op1 = inst.getOperand(0);
		llvm::Value *op2 = nullptr;
		if (inst.getNumOperands() > 1)
		{
			op2 = inst.getOperand(1);
		}
		switch (opCode)
		{
		case llvm::Instruction::Store:
			if (opTrace[op2] != nullptr)
			{
				for (auto &p : *(opTrace[op2]))
				{
					delete commonSubexpressions[p];
					commonSubexpressions.erase(p);
				}
				delete opTrace[op2];
				opTrace.erase(op2);
			}
			break;
		case llvm::Instruction::Alloca:
		case llvm::Instruction::Br:
			break;
		default:
			llvm::Value *op2Ref = op2 != nullptr ? op2 : op1;
			pair<llvm::Value *, llvm::Value *> operands = make_pair(max(op1, op2Ref), min(op1, op2Ref));
			pair<unsigned, llvm::Instruction *> *subExpr = commonSubexpressions[operands];
			if (subExpr != nullptr && subExpr->first == opCode)
			{
				log(string{"CSE -> "} + getInstructionString(inst));
				inst.replaceAllUsesWith(subExpr->second);
				change = true;
			}
			else
			{
				commonSubexpressions[operands] = new pair<unsigned, llvm::Instruction *>(opCode, &inst);
				if (opTrace[op1] == nullptr)
				{
					opTrace[op1] = new vector<pair<llvm::Value *, llvm::Value *>>();
				}
				opTrace[op1]->push_back(operands);
				if (op2 != NULL)
				{
					if (opTrace[op2] == nullptr)
					{
						opTrace[op2] = new vector<pair<llvm::Value *, llvm::Value *>>();
					}
					opTrace[op2]->push_back(operands);
				}
			}
			break;
		}
	}
}

void eliminateDeadCode(llvm::BasicBlock &basicBlock, bool &change)
{
	vector<llvm::Instruction *> toErase;
	for (llvm::Instruction &inst : basicBlock)
	{
		unsigned opCode = inst.getOpcode();
		bool check = (opCode != llvm::Instruction::Store &&
									opCode != llvm::Instruction::Alloca &&
									opCode != llvm::Instruction::Br &&
									opCode != llvm::Instruction::Call &&
									opCode != llvm::Instruction::Ret);
		if (inst.hasNUses(0) && check)
		{
			toErase.push_back(&inst);
			change = true;
		}
	}
	for (llvm::Instruction *inst : toErase)
	{
		log(string{"DE -> "} + getInstructionString(*inst));
		inst->eraseFromParent();
	}
}

void constantFolding(llvm::BasicBlock &basicBlock, bool &change)
{
	for (llvm::Instruction &inst : basicBlock)
	{
		llvm::Value *op1 = inst.getOperand(0);
		llvm::Value *op2 = nullptr;
		if (inst.getNumOperands() > 1)
		{
			op2 = inst.getOperand(1);
		}
		else
		{
			continue;
		}
		llvm::ConstantInt *const1 = dyn_cast<llvm::ConstantInt>(op1);
		llvm::ConstantInt *const2 = dyn_cast<llvm::ConstantInt>(op2);
		unsigned opCode = inst.getOpcode();
		llvm::CmpInst::Predicate predicate;
		if (const1 && const2)
		{
			log(string{"CF  -> "} + getInstructionString(inst));
			llvm::Constant *newInstruction = nullptr;
			switch (opCode)
			{
			case llvm::Instruction::Add:
				newInstruction = llvm::ConstantExpr::getAdd(const1, const2);
				break;
			case llvm::Instruction::Sub:
				newInstruction = llvm::ConstantExpr::getSub(const1, const2);
				break;
			case llvm::Instruction::Mul:
				newInstruction = llvm::ConstantExpr::getMul(const1, const2);
				break;
			case llvm::Instruction::ICmp:
				predicate = llvm::cast<llvm::ICmpInst>(&inst)->getPredicate();
				newInstruction = llvm::ConstantExpr::getICmp(predicate, const1, const2);
				break;
			default:
				break;
			}
			if (newInstruction)
			{
				inst.replaceAllUsesWith(newInstruction);
				change = true;
			}
		}
	}
}

void constantPropagation(llvm::Function &func, bool &change)
{
	map<llvm::Value *, llvm::ConstantInt *> constants;
	vector<llvm::Instruction *> toErase;
	for (llvm::BasicBlock &basicBlock : func)
	{
		for (llvm::Instruction &inst : basicBlock)
		{
			unsigned opCode = inst.getOpcode();
			llvm::Value *op1 = inst.getOperand(0);
			llvm::Value *op2 = nullptr;
			if (inst.getNumOperands() > 1)
			{
				op2 = inst.getOperand(1);
			}
			llvm::ConstantInt *const1 = dyn_cast<llvm::ConstantInt>(op1);
			switch (opCode)
			{
			case llvm::Instruction::Store:
				if (const1)
				{
					if (constants[op2] != nullptr && const1->getValue() == constants[op2]->getValue())
					{
						toErase.push_back(&inst);
					}
					else
					{
						constants[op2] = const1;
					}
				}
				else if (constants[op2] != nullptr)
				{
					constants.erase(op2);
				}
				break;
			case llvm::Instruction::Load:
				if (const1)
				{
					log(string{"CP -> "} + getInstructionString(inst));
					inst.replaceAllUsesWith(const1);
					change = true;
				}
				else if (constants[op1] != nullptr)
				{
					log(string{"CP -> "} + getInstructionString(inst));
					inst.replaceAllUsesWith(constants[op1]);
					change = true;
				}
			default:
				break;
			}
		}
	}
	for (llvm::Instruction *inst : toErase)
	{
		log(string{"CSE -> "} + getInstructionString(*inst));
		inst->eraseFromParent();
	}
}

unique_ptr<llvm::Module> createModule(string irFileName, llvm::LLVMContext &context)
{
	llvm::SMDiagnostic error;
	unique_ptr<llvm::Module> module = llvm::parseIRFile(irFileName, error, context);
	if (!module)
	{
		llvm::errs() << "Error reading IR file " << irFileName << ":\n";
		error.print("createModule", llvm::errs());
		exit(1);
	}
	return module;
}

void optimizeModule(llvm::Module &module)
{
	for (llvm::Function &func : module.functions())
	{

		bool change;
		do
		{
			change = false;
			for (llvm::BasicBlock &block : func)
			{
				eliminateCommonSubExpression(block, change);
				eliminateDeadCode(block, change);
				constantFolding(block, change);
			}
			constantPropagation(func, change);
		} while (change);
	}
}

void saveModule(unique_ptr<llvm::Module> &module, const string fileName)
{
	ofstream out(fileName);
	llvm::raw_os_ostream os(out);
	module->print(os, nullptr);
}

void runOptimizer(string irInFileName, string irOutFileName)
{
	llvm::LLVMContext context;
	unique_ptr<llvm::Module> module = createModule(irInFileName, context);
	if (module)
	{
		optimizeModule(*(module.get()));
		if (irOutFileName.size())
		{
			saveModule(module, irOutFileName);
		}
		else
		{
			module->print(llvm::outs(), nullptr);
		}
		module.reset();
	}
}

astNode *getBlockEnd(astNode *node)
{
	if (node->type == ast_stmt && node->stmt.type == ast_block)
	{
		return node->stmt.block.stmt_list->back();
	}
	return node;
}

llvm::Value *parseExpression(llvm::IRBuilder<> &builder, astNode *node, map<string, llvm::Value *> allocaMap)
{
	assert(node);
	switch (node->type)
	{
	case ast_bexpr:
	{
		llvm::Value *lhs = parseExpression(builder, node->bexpr.lhs, allocaMap);
		llvm::Value *rhs = parseExpression(builder, node->bexpr.rhs, allocaMap);
		llvm::Instruction::BinaryOps op;

		switch (node->bexpr.op)
		{
		case add:
			op = llvm::Instruction::Add;
			break;
		case sub:
			op = llvm::Instruction::Sub;
			break;
		case mul:
			op = llvm::Instruction::Mul;
			break;
		case divide:
			op = llvm::Instruction::SDiv;
			break;
		default:
			break;
		}
		return builder.CreateBinOp(op, lhs, rhs);
	}
	case ast_uexpr:
	{
		llvm::Value *uexpr = parseExpression(builder, node->uexpr.expr, allocaMap);
		op_type op = node->uexpr.op;
		switch (op)
		{
		case add:
		{
			return uexpr;
		}
		case sub:
		{
			return builder.CreateUnOp(llvm::Instruction::UnaryOps::UnaryOpsEnd, uexpr);
		}
		default:
			break;
		}
	}
	case ast_cnst:
	{
		return builder.getInt32(node->cnst.value);
	}
	case ast_var:
	{
		llvm::Value *allocaP = allocaMap[string{node->var.name}];
		return builder.CreateLoad(builder.getInt32Ty(), allocaP, false);
	}
	default:
		break;
	}
	return nullptr;
}

void llvmBackendCaller(astNode *node)
{
	if (node->type != ast_prog)
	{
		cerr << "Wrong type or astNode. Must begin with an ast_prog" << endl;
		exit(1);
	}
	llvm::LLVMContext context;
	llvm::Module *module = new llvm::Module("Prog", context);
	;
	llvm::IRBuilder<> builder(context);

	astProg prog = node->prog;
	astFunc func = prog.func->func;

	llvm::FunctionType *funcType;
	llvm::Type *retType = string{func.type} == string{"int"} ? builder.getInt32Ty() : builder.getVoidTy();
	if (func.param != nullptr)
	{
		funcType = llvm::FunctionType::get(retType, builder.getInt32Ty(), false);
	}
	else
	{
		funcType = llvm::FunctionType::get(retType, false);
	}
	llvm::Function *llvmFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, func.name, module);

	vector<astNode *> nodeStack;
	nodeStack.push_back(func.body);
	vector<vector<astNode *>> blockStack;
	map<astNode *, tuple<llvm::BasicBlock *, llvm::BasicBlock *>> blockSuccessionMap;
	map<string, llvm::Value *> allocaMap;
	map<astNode *, llvm::BasicBlock *> blockNodeMap;
	llvm::BasicBlock *fb = nullptr;
	while (nodeStack.size() || blockStack.size())
	{
		astNode *node = nodeStack.back();
		nodeStack.pop_back();
		if (node == nullptr)
		{
			continue;
		}
		switch (node->type)
		{
		case ast_stmt:
			switch (node->stmt.type)
			{
			case ast_block:
			{
				llvm::BasicBlock *block;
				if (blockNodeMap[node])
				{
					block = blockNodeMap[node];
				}
				else
				{
					block = llvm::BasicBlock::Create(context, "", llvmFunc);
				}
				builder.SetInsertPoint(block);
				if (llvmFunc->size() == 1 && func.param != nullptr)
				{
					astVar var = func.param->var;
					llvm::Value *allocaP = builder.CreateAlloca(llvm::Type::getInt32Ty(context), 0, nullptr, string{var.name});
					llvm::Value *funcParamValue = llvmFunc->arg_begin();
					builder.CreateStore(builder.CreateLoad(funcParamValue->getType(), funcParamValue), allocaP, false);
					allocaMap[string{var.name}] = allocaP;
				}
				else if (llvmFunc->size() > 1)
				{
					blockStack.push_back(nodeStack);
					nodeStack = vector<astNode *>{};
				}
				vector<astNode *> stmts = *(node->stmt.block.stmt_list);
				for (auto it = stmts.rbegin(); it != stmts.rend(); it++)
				{
					nodeStack.push_back(*it);
				}
				break;
			}
			case ast_asgn:
			{
				llvm::Value *lhs = allocaMap[string{node->stmt.asgn.lhs->var.name}];
				llvm::Value *rhs = parseExpression(builder, node->stmt.asgn.rhs, allocaMap);
				builder.CreateStore(lhs, rhs, false);
				break;
			}
			case ast_while:
				break;
			case ast_if:
			{
				astIf ifNode = node->stmt.ifn;
				astRExpr rexpr = ifNode.cond->rexpr;
				llvm::Value *lhs = parseExpression(builder, rexpr.lhs, allocaMap);
				llvm::Value *rhs = parseExpression(builder, rexpr.rhs, allocaMap);
				;
				llvm::Value *cond;
				switch (rexpr.op)
				{
				case lt:
					cond = builder.CreateICmpSLT(lhs, rhs, "");
					break;
				case gt:
					cond = builder.CreateICmpSGT(lhs, rhs, "");
					break;
				case le:
					cond = builder.CreateICmpSLE(lhs, rhs, "");
					break;
				case ge:
					cond = builder.CreateICmpSGE(lhs, rhs, "");
					break;
				case eq:
					cond = builder.CreateICmpEQ(lhs, rhs, "");
					break;
				case neq:
					cond = builder.CreateICmpNE(lhs, rhs, "");
					break;
				default:
					break;
				}

				llvm::BasicBlock *ifBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
				llvm::BasicBlock *elseBlock = nullptr;
				blockNodeMap[ifNode.if_body] = ifBlock;
				if (ifNode.else_body)
				{
					elseBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					llvm::BasicBlock *finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					fb = finalBlock;
					builder.CreateCondBr(cond, ifBlock, elseBlock);
					blockSuccessionMap[getBlockEnd(ifNode.if_body)] = make_tuple(elseBlock, finalBlock);
					blockSuccessionMap[getBlockEnd(ifNode.else_body)] = make_tuple(finalBlock, finalBlock);
					blockNodeMap[ifNode.else_body] = elseBlock;
					nodeStack.push_back(ifNode.else_body);
				}
				else
				{
					llvm::BasicBlock *finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					fb = finalBlock;
					builder.CreateCondBr(cond, ifBlock, finalBlock);
					blockSuccessionMap[getBlockEnd(ifNode.if_body)] = make_tuple(finalBlock, finalBlock);
				}
				nodeStack.push_back(ifNode.if_body);
			}
			break;
			case ast_ret:
			{
				llvm::Value *retexpr = parseExpression(builder, node->stmt.ret.expr, allocaMap);
				builder.CreateRet(retexpr);
				break;
			}
			case ast_call:
				break;
			case ast_decl:
			{
				string varName{node->stmt.decl.name};
				llvm::Value *allocaP = builder.CreateAlloca(builder.getInt32Ty(), nullptr, varName);
				allocaMap[varName] = allocaP;
				break;
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
		if (nodeStack.empty() && !blockStack.empty())
		{
			nodeStack = blockStack.back();
			blockStack.pop_back();
		}
		if (blockSuccessionMap.find(node) != blockSuccessionMap.end())
		{
			cout << node << " in here -> " << fb << endl;
			tuple<llvm::BasicBlock *, llvm::BasicBlock *> nextBr = blockSuccessionMap[node];
			builder.CreateBr(get<1>(nextBr));
			cout << "next -> " << get<0>(nextBr) << endl;
			builder.SetInsertPoint(get<0>(nextBr));
			cout << "br -> " << get<1>(nextBr) << endl;
		}
	}
	module->print(llvm::outs(), nullptr);
	optimizeModule(*module);
	module->print(llvm::outs(), nullptr);
}

#ifdef OPTIMIZER
int main(int argc, char **argv)
{
	if (argc >= 2)
	{
		runOptimizer(string{argv[1]}, argc == 3 ? string{argv[2]} : string{""});
	}
	return 0;
}
#endif