/**
 * @file optimizer.cpp
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

void log(string s)
{
#ifdef LOG
	cout << s << endl;
#endif
}

void generateKillGen(set<llvm::Instruction *> &gen,
										 set<llvm::Instruction *> &kill,
										 map<llvm::Value *, set<llvm::Value *> *> &store,
										 llvm::BasicBlock &block)
{
	for (llvm::Instruction &inst : block)
	{
		unsigned opCode = inst.getOpcode();
		if (opCode == llvm::Instruction::Store)
		{
			gen.insert(&inst);
			llvm::Value *op1 = inst.getOperand(0);
			llvm::Value *op2 = inst.getOperand(1);
			if (store[op2] != nullptr)
			{
				set<llvm::Value *> *killed = store[op2];
				killed->insert(killed->begin(), killed->end());
			}
			store[op2] = new set<llvm::Value *>();
			store[op2]->insert(&inst);
		}
	}
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
		if (inst.getNumOperands() == 0)
		{
			continue;
		}
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
	set<llvm::Instruction *> toErase;
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
			toErase.insert(&inst);
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
		if (inst.getNumOperands() == 0)
		{
			continue;
		}
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
	map<llvm::BasicBlock *, set<llvm::Instruction *>> kill, gen, ins, outs;
	map<llvm::Value *, set<llvm::Value *> *> store;

	// generate kill gen
	for (llvm::BasicBlock &block : func)
	{
		generateKillGen(gen[&block], kill[&block], store, block);
		outs[&block] = gen[&block];
	}
	for (llvm::BasicBlock &block : func)
	{
		bool tempChange = true;
		do
		{
			tempChange = false;
			// union of predecessor of block
			for_each(
					llvm::pred_begin(&block),
					llvm::pred_end(&block),
					[&ins, &outs, &block](llvm::BasicBlock *pred)
					{
						set<llvm::Instruction *> bOut = outs[pred];
						ins[&block].insert(bOut.begin(), bOut.end());
					});

			// difference of in and kill
			set<llvm::Instruction *> oldOut = outs[&block];
			set<llvm::Instruction *> diff;
			for (llvm::Instruction *inst : ins[&block])
			{
				if (kill[&block].find(inst) == kill[&block].end())
				{
					diff.insert(inst);
				}
			}

			// union(out, diff(in, kill))
			outs[&block] = gen[&block];
			outs[&block].insert(diff.begin(), diff.end());

			// diff(out, old_out)
			if (outs[&block].size() != oldOut.size())
			{
				tempChange = true;
			}
			// break;
		} while (tempChange);
	}

	set<llvm::Instruction *> toErase;
	for (llvm::BasicBlock &block : func)
	{
		set<llvm::Instruction *> R = ins[&block];
		for (llvm::Instruction &inst : block)
		{
			unsigned opCode = inst.getOpcode();
			if (opCode == llvm::Instruction::Store)
			{
				erase_if(R, [&inst](llvm::Instruction *x)
								 { return x->getOperand(1) == inst.getOperand(1); });
				R.insert(&inst);
			}
			if (opCode == llvm::Instruction::Load)
			{
				llvm::ConstantInt *constVal = nullptr;
				find_if(R,
								[&inst, &toErase](llvm::Instruction *x)
								{
									llvm::ConstantInt *constOp = dyn_cast<llvm::ConstantInt>(x->getOperand(0));
									if (constOp != nullptr && inst.getOperand(0) == x->getOperand(1))
									{
										inst.replaceAllUsesWith((llvm::Value *)constOp);
										toErase.insert(&inst);
									}
									return false;
								});
			}
		}
	}
	for (llvm::Instruction *inst : toErase)
	{
		inst->eraseFromParent();
	}
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
