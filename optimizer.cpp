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

#include "optimizer.h"

using namespace std;
llvm::BasicBlock *getNthBlock(llvm::Function &func, unsigned int n)
{
	if (func.size() == 0)
	{
		return nullptr;
	}
	llvm::BasicBlock *block = &func.getEntryBlock();
	for (unsigned int i = 0; i < n && block != nullptr; ++i)
	{
		block = block->getNextNode();
	}
	return block;
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

void optimizeModule(unique_ptr<llvm::Module> &module)
{
	for (llvm::Function &func : module->functions())
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
		optimizeModule(module);
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

llvm::Value *generateIR(astNode *node, llvm::LLVMContext &context, llvm::IRBuilder<> &builder, llvm::Function *func, int &blockNum)
{
	llvm::Value *res = nullptr;
	llvm::BasicBlock *block;
	llvm::Value *inst1;
	llvm::Value *inst2;
	llvm::Instruction::BinaryOps opCode;
	if (node == nullptr)
	{
		return res;
	}

	switch (node->type)
	{
	case ast_stmt:
		switch (node->stmt.type)
		{
		case ast_block:
			block = llvm::BasicBlock::Create(context, to_string(func->size()), func);
			blockNum++;
			for (astNode *stmt : *(node->stmt.block.stmt_list))
			{
				generateIR(stmt, context, builder, func);
			}
			blockNum--;
			res = block->getFirstNonPHIOrDbg();
			break;
		case ast_asgn:
			inst1 = generateIR(node->stmt.asgn.lhs, context, builder, func);
			inst2 = generateIR(node->stmt.asgn.rhs, context, builder, func);
			res = llvm::cast<llvm::Value>(builder.CreateStore(inst1, inst2));
			break;
		case ast_while:
			break;
		case ast_if:
			break;
		case ast_ret:
			break;
		case ast_call:
			break;
		case ast_decl:
			break;
		default:
			break;
		}
		break;
	case ast_var:
		break;
	case ast_bexpr:
		inst1 = generateIR(node->stmt.asgn.lhs, context, builder, func);
		inst2 = generateIR(node->stmt.asgn.rhs, context, builder, func);

		switch (node->bexpr.op)
		{
		case add:
			opCode = llvm::Instruction::Add;
			break;
		case sub:
			opCode = llvm::Instruction::Sub;
			break;
		case mul:
			opCode = llvm::Instruction::Mul;
			break;
		case divide:
			opCode = llvm::Instruction::SDiv;
			break;
		default:
			break;
		}
		if (opCode) {
			res = llvm::cast<llvm::Value>(builder.CreateBinOp(opCode, inst1, inst1, ""));
		}
		break;
	case ast_rexpr:
		break;
	case ast_uexpr:
		break;
	default:
		break;
	}
	return res;
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

	astFunc func = node->func;

	llvm::FunctionType *funcType;
	llvm::Type *retType = string{func.type} == string{"int"} ? builder.getInt32Ty() : builder.getVoidTy();
	if (func.param == nullptr)
	{
		funcType = llvm::FunctionType::get(retType, builder.getInt32Ty(), false);
	}
	else
	{
		funcType = llvm::FunctionType::get(retType, false);
	}
	llvm::Function *llvmFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, func.name, module);
}

int main(int argc, char **argv)
{
	if (argc >= 2)
	{
		runOptimizer(string{argv[1]}, argc == 3 ? string{argv[2]} : string{""});
	}
	return 0;
}