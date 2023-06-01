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
using namespace llvm;

inline void log(string s)
{
#ifdef LOG
  cout << s << endl;
#endif
}

void generateKillGen(set<Instruction *> &gen,
										 set<Instruction *> &kill,
										 map<Value *, set<Value *> *> &store,
										 BasicBlock &block)
{
	for (Instruction &inst : block)
	{
		unsigned opCode = inst.getOpcode();
		if (opCode == Instruction::Store)
		{
			gen.insert(&inst);
			Value *op1 = inst.getOperand(0);
			Value *op2 = inst.getOperand(1);
			if (store[op2] != nullptr)
			{
				set<Value *> *killed = store[op2];
				killed->insert(killed->begin(), killed->end());
			}
			store[op2] = new set<Value *>();
			store[op2]->insert(&inst);
		}
	}
}

string getInstructionString(Instruction &inst)
{
	string instStr;
	raw_string_ostream(instStr) << inst;
	return instStr;
}

void eliminateCommonSubExpression(BasicBlock &basicBlock, bool &change)
{
	map<pair<Value *, Value *>, pair<unsigned, Instruction *> *> commonSubexpressions;
	map<Value *, vector<pair<Value *, Value *>> *> opTrace;

	for (Instruction &inst : basicBlock)
	{
		unsigned opCode = inst.getOpcode();
		if (inst.getNumOperands() == 0)
		{
			continue;
		}
		Value *op1 = inst.getOperand(0);
		Value *op2 = nullptr;

		if (inst.getNumOperands() > 1)
		{
			op2 = inst.getOperand(1);
		}
		switch (opCode)
		{
		case Instruction::Store:
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
		case Instruction::Alloca:
		case Instruction::Br:
		case Instruction::Call:
			break;
		default:
			Value *op2Ref = op2 != nullptr ? op2 : op1;
			pair<Value *, Value *> operands = make_pair(max(op1, op2Ref), min(op1, op2Ref));
			pair<unsigned, Instruction *> *subExpr = commonSubexpressions[operands];
			if (subExpr != nullptr && subExpr->first == opCode)
			{
				log(string{"CSE -> "} + getInstructionString(inst));
				inst.replaceAllUsesWith(subExpr->second);
				change = true;
			}
			else
			{
				commonSubexpressions[operands] = new pair<unsigned, Instruction *>(opCode, &inst);
				if (opTrace[op1] == nullptr)
				{
					opTrace[op1] = new vector<pair<Value *, Value *>>();
				}
				opTrace[op1]->push_back(operands);
				if (op2 != NULL)
				{
					if (opTrace[op2] == nullptr)
					{
						opTrace[op2] = new vector<pair<Value *, Value *>>();
					}
					opTrace[op2]->push_back(operands);
				}
			}
			break;
		}
	}
}

void eliminateDeadCode(BasicBlock &basicBlock, bool &change)
{
	set<Instruction *> toErase;
	for (Instruction &inst : basicBlock)
	{
		unsigned opCode = inst.getOpcode();
		bool check = (opCode != Instruction::Store &&
									opCode != Instruction::Alloca &&
									opCode != Instruction::Br &&
									opCode != Instruction::Call &&
									opCode != Instruction::Ret);
		if (inst.hasNUses(0) && check)
		{
			toErase.insert(&inst);
			change = true;
		}
	}
	for (Instruction *inst : toErase)
	{
		log(string{"DE -> "} + getInstructionString(*inst));
		inst->eraseFromParent();
	}
}

void constantFolding(BasicBlock &basicBlock, bool &change)
{
	for (Instruction &inst : basicBlock)
	{
		if (inst.getNumOperands() == 0)
		{
			continue;
		}
		Value *op1 = inst.getOperand(0);
		Value *op2 = nullptr;
		if (inst.getNumOperands() > 1)
		{
			op2 = inst.getOperand(1);
		}
		else
		{
			continue;
		}
		ConstantInt *const1 = dyn_cast<ConstantInt>(op1);
		ConstantInt *const2 = dyn_cast<ConstantInt>(op2);
		unsigned opCode = inst.getOpcode();
		CmpInst::Predicate predicate;
		if (const1 && const2)
		{
			log(string{"CF  -> "} + getInstructionString(inst));
			Constant *newInstruction = nullptr;
			switch (opCode)
			{
			case Instruction::Add:
				newInstruction = ConstantExpr::getAdd(const1, const2);
				break;
			case Instruction::Sub:
				newInstruction = ConstantExpr::getSub(const1, const2);
				break;
			case Instruction::Mul:
				newInstruction = ConstantExpr::getMul(const1, const2);
				break;
			case Instruction::ICmp:
				predicate = cast<ICmpInst>(&inst)->getPredicate();
				newInstruction = ConstantExpr::getICmp(predicate, const1, const2);
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

void constantPropagation(Function &func, bool &change)
{
	map<BasicBlock *, set<Instruction *>> kill, gen, ins, outs;
	map<Value *, set<Value *> *> store;

	// generate kill gen
	for (BasicBlock &block : func)
	{
		generateKillGen(gen[&block], kill[&block], store, block);
		outs[&block] = gen[&block];
	}
	for (BasicBlock &block : func)
	{
		bool tempChange = true;
		do
		{
			tempChange = false;
			// union of predecessor of block
			for_each(
					pred_begin(&block),
					pred_end(&block),
					[&ins, &outs, &block](BasicBlock *pred)
					{
						set<Instruction *> bOut = outs[pred];
						ins[&block].insert(bOut.begin(), bOut.end());
					});

			// difference of in and kill
			set<Instruction *> oldOut = outs[&block];
			set<Instruction *> diff;
			for (Instruction *inst : ins[&block])
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

	set<Instruction *> toErase;
	for (BasicBlock &block : func)
	{
		set<Instruction *> R = ins[&block];
		for (Instruction &inst : block)
		{
			unsigned opCode = inst.getOpcode();
			if (opCode == Instruction::Store)
			{
				erase_if(R, [&inst](Instruction *x)
								 { return x->getOperand(1) == inst.getOperand(1); });
				R.insert(&inst);
			}
			if (opCode == Instruction::Load)
			{
				ConstantInt *constVal = nullptr;
				bool replace{false};

				for_each(R,
								 [&inst, &constVal, &replace](Instruction *x)
								 {
									 ConstantInt *constOp = dyn_cast<ConstantInt>(x->getOperand(0));
									 if (constOp != nullptr)
									 {
										 if (
												 constVal != nullptr && inst.getOperand(0) == x->getOperand(1) || inst.getOperand(0) == x->getOperand(1))
										 {
											 constVal = constOp;
											 replace = true;
										 }
									 }
									 else
									 {
										 replace = false;
									 }
								 });
				if (constVal != nullptr && replace)
				{
					inst.replaceAllUsesWith((Value *)constVal);
					toErase.insert(&inst);
				}
			}
		}
	}
	for_each(toErase, [&change](Instruction *inst)
					 { inst->eraseFromParent(); 
					 change = true; });
}

void optimizeModule(Module &module)
{
	for (Function &func : module.functions())
	{

		bool change;
		do
		{
			change = false;
			for (BasicBlock &block : func)
			{
				eliminateCommonSubExpression(block, change);
				eliminateDeadCode(block, change);
				constantFolding(block, change);
			}
			constantPropagation(func, change);
		} while (change);
	}
}
