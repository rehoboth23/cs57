/**
 * @file opt.h
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
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <llvm-c/Core.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Transforms/Scalar.h>
#include <map>
#include <vector>

using namespace std;

void eliminateCommonSubExpression(LLVMBasicBlockRef bb)
{
	std::map<pair<LLVMValueRef, LLVMValueRef>, pair<LLVMOpcode, LLVMValueRef> *> commonSubexpressions;
	std::map<LLVMValueRef, vector<pair<LLVMValueRef, LLVMValueRef>> *> opTrace;
	std::map<LLVMValueRef, LLVMValueRef> valueMap;

	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL;)
	{
		LLVMOpcode opCode = LLVMGetInstructionOpcode(inst);
		LLVMValueRef op1;
		LLVMValueRef op2;
		op1 = LLVMGetOperand(inst, 0);
		op2 = LLVMGetOperand(inst, 1);
		switch (opCode)
		{
		case LLVMStore:
			if (opTrace[op2] != nullptr)
			{
				for (pair<LLVMValueRef, LLVMValueRef> p : *(opTrace[op2]))
				{
					commonSubexpressions.erase(p);
				}
			}
			break;
		case LLVMAlloca:
		case LLVMBr:
			break;
		case LLVMLoad:
			valueMap[inst] = op1;
			break;
		default:
			if (valueMap[op1] != NULL)
			{
				op1 = valueMap[op1];
			}
			if (op2 != NULL && valueMap[op2] != NULL)
			{
				op2 = valueMap[op2];
			}
			pair<LLVMValueRef, LLVMValueRef> operands;
			LLVMValueRef op2Ref = op2 != NULL ? op2 : op1;
			operands = make_pair(max(op1, op2Ref), min(op1, op2Ref));
			pair<LLVMOpcode, LLVMValueRef> *subExpr = commonSubexpressions[operands];
			if (subExpr != nullptr && subExpr->first == opCode)
			{
				LLVMReplaceAllUsesWith(inst, subExpr->second);
			}
			else
			{
				commonSubexpressions[operands] = new pair<LLVMOpcode, LLVMValueRef>(opCode, inst);
				if (opTrace[op1] == nullptr)
				{
					opTrace[op1] = new vector<pair<LLVMValueRef, LLVMValueRef>>();
				}
				opTrace[op1]->push_back(operands);
				if (op2 != NULL)
				{
					if (opTrace[op2] == nullptr)
					{
						opTrace[op2] = new vector<pair<LLVMValueRef, LLVMValueRef>>();
					}
					opTrace[op2]->push_back(operands);
				}
			};
		}
		inst = LLVMGetNextInstruction(inst);
	}
}

void eliminateDeadCode(LLVMBasicBlockRef bb)
{
	bool res = false;
	do
	{
		res = false;
		for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
		{
			LLVMOpcode opCode = LLVMGetInstructionOpcode(inst);
			bool check = (opCode != LLVMStore &&
										opCode != LLVMAlloca &&
										opCode != LLVMBr &&
										opCode != LLVMCall &&
										opCode != LLVMRet);
			if (LLVMGetFirstUse(inst) == NULL && check)
			{
				LLVMInstructionEraseFromParent(inst);
				res = true;
			}
		}
	} while (res);
}

// doesn't seem to change anything
void constantFolding(LLVMBasicBlockRef bb)
{
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
	{
		LLVMValueRef op1 = LLVMGetOperand(inst, 0);
		LLVMValueRef op2 = LLVMGetOperand(inst, 1);
		LLVMOpcode opCode = LLVMGetInstructionOpcode(inst);
		LLVMBool c1 = LLVMIsConstant(op1);
		LLVMBool c2 = LLVMIsConstant(op1);
		if (c1 && c1)
		{
			LLVMValueRef newInstruction = NULL;
			switch (opCode)
			{
			case LLVMAdd:
				newInstruction = LLVMConstAdd(op1, op2);
				break;
			case LLVMSub:
				cout << "folding sub" << endl;
				newInstruction = LLVMConstSub(op1, op2);
				break;
			case LLVMMul:
				cout << "folding mul" << endl;
				newInstruction = LLVMConstMul(op1, op2);
				break;
			default:
				break;
			}
			if (newInstruction)
			{
				LLVMReplaceAllUsesWith(inst, newInstruction);
			}
		}
	}
}
