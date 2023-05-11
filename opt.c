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
#include <map>
#include <vector>
#include "opt.h"

using namespace std;

void eliminateCommonSubExpression(LLVMBasicBlockRef bb, bool *change)
{
	std::map<pair<LLVMValueRef, LLVMValueRef>, pair<LLVMOpcode, LLVMValueRef> *> commonSubexpressions;
	std::map<LLVMValueRef, vector<pair<LLVMValueRef, LLVMValueRef>> *> opTrace;

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
		default:
			pair<LLVMValueRef, LLVMValueRef> operands;
			LLVMValueRef op2Ref = op2 != NULL ? op2 : op1;
			operands = make_pair(max(op1, op2Ref), min(op1, op2Ref));
			pair<LLVMOpcode, LLVMValueRef> *subExpr = commonSubexpressions[operands];
			if (subExpr != nullptr && subExpr->first == opCode)
			{
				LLVMReplaceAllUsesWith(inst, subExpr->second);
				cout << "CSE -> " << LLVMPrintValueToString(inst) << endl;
				*change = true;
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

void eliminateDeadCode(LLVMBasicBlockRef bb, bool *change)
{
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
	{
		LLVMOpcode opCode = LLVMGetInstructionOpcode(inst);
		bool check = (opCode != LLVMStore &&
									opCode != LLVMAlloca &&
									opCode != LLVMBr &&
									opCode != LLVMCall &&
									opCode != LLVMRet);
		// cout << "DCE" << endl;
		if (LLVMGetFirstUse(inst) == NULL && check)
		{
			cout << "DCE -> " << LLVMPrintValueToString(inst) << endl;
			LLVMInstructionEraseFromParent(inst);
			*change = true;
		}
	}
}

// doesn't seem to change anything
void constantFolding(LLVMBasicBlockRef bb, bool *change)
{
	for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != NULL; inst = LLVMGetNextInstruction(inst))
	{
		LLVMValueRef op1 = LLVMGetOperand(inst, 0);
		LLVMValueRef op2 = LLVMGetOperand(inst, 1);
		LLVMOpcode opCode = LLVMGetInstructionOpcode(inst);
		LLVMIntPredicate predicate;
		if (LLVMIsAConstantInt(op1) && LLVMIsAConstantInt(op2))
		{
			cout << "CF  -> " << LLVMPrintValueToString(inst) << endl;
			LLVMValueRef newInstruction = NULL;
			switch (opCode)
			{
			case LLVMAdd:
				newInstruction = LLVMConstAdd(op1, op2);
				*change = true;
				break;
			case LLVMSub:
				newInstruction = LLVMConstSub(op1, op2);
				*change = true;
				break;
			case LLVMMul:
				newInstruction = LLVMConstMul(op1, op2);
				*change = true;
				break;
			case LLVMICmp:
				predicate = LLVMGetICmpPredicate(inst);
				newInstruction = LLVMConstICmp(predicate, op1, op2);
				*change = true;
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

void constantPropagation(LLVMValueRef function, bool *change)
{
	map<LLVMValueRef, LLVMValueRef> constants;
	for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
			 basicBlock;
			 basicBlock = LLVMGetNextBasicBlock(basicBlock))
	{
		for (LLVMValueRef inst = LLVMGetFirstInstruction(basicBlock); inst != NULL; inst = LLVMGetNextInstruction(inst))
		{
			LLVMOpcode opCode = LLVMGetInstructionOpcode(inst);
			LLVMValueRef op1 = LLVMGetOperand(inst, 0);
			LLVMValueRef op2 = LLVMGetOperand(inst, 1);
			switch (opCode)
			{
			case LLVMStore:
				if (LLVMIsAConstantInt(op1))
				{
					if (constants[op2] != nullptr && LLVMConstIntGetSExtValue(op1) == LLVMConstIntGetSExtValue(constants[op2]))
					{
						// LLVMReplaceAllUsesWith(op1, constants[op2]);
						cout << "DFE/CP  -> " << LLVMPrintValueToString(inst) << endl;
						LLVMInstructionEraseFromParent(inst);
						*change = true;
					}
					else
					{
						constants[op2] = op1;
					}
				}
				else if (constants[op2] != nullptr)
				{
					constants.erase(op2);
				}
				break;
			case LLVMLoad:
				if (LLVMIsAConstantInt(op1))
				{
					cout << "CP  -> " << LLVMPrintValueToString(inst) << endl;
					int64_t op1Val = LLVMConstIntGetSExtValue(op1);
					LLVMValueRef newInstruction = LLVMConstInt(LLVMInt32Type(), op1Val, 1);
					LLVMReplaceAllUsesWith(inst, newInstruction);
					*change = true;
				}
				else if (constants[op1] != nullptr)
				{
					cout << "CP  -> " << LLVMPrintValueToString(inst) << endl;
					int64_t op1Val = LLVMConstIntGetSExtValue(constants[op1]);
					LLVMValueRef newInstruction = LLVMConstInt(LLVMInt32Type(), op1Val, 1);
					LLVMReplaceAllUsesWith(inst, newInstruction);
					*change = true;
				}
			default:
				break;
			}
		}
	}
}

void saveModule(LLVMModuleRef module, const char *filename)
{
	char *errorMessage = NULL;
	if (filename == NULL)
	{
		cout << "hello world" << endl;
		LLVMDumpModule(module);
		return;
	}
	FILE *fp = fopen(filename, "w");
	fclose(fp);
	int result = LLVMPrintModuleToFile(module, filename, &errorMessage);
	if (result != 0)
	{
		fprintf(stderr, "Failed to write module to file: %s\n", errorMessage);
		LLVMDisposeMessage(errorMessage);
	}
	else
	{
		printf("Module saved to file %s\n", filename);
	}
}