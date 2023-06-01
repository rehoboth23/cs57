/**
 * @file ir_gen.cpp
 * @author Rehoboth Okorie
 * @brief IR generation
 *
 * @version 0.1
 * @date 2023-05-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "ir_gen.h"

using namespace std;
using namespace llvm;

#ifdef ARMD
string ARM_FUNC_PRE = "_";
#else
string ARM_FUNC_PRE = "";
#endif

void moveTop(Instruction *&top, Instruction *toMove)
{
	toMove->moveAfter(top);
	top = toMove;
}

Type *getType(var_type type, IRBuilder<> &builder)
{
	switch (type)
	{
	case int_ty:
		return builder.getInt32Ty();
		break;
	case char_ty:
		return builder.getInt8Ty();
		break;
	case ptr_ty:
		return builder.getInt8PtrTy();
		break;
	default:
		return builder.getVoidTy();
		break;
	}
}

Value *parseExpression(IRBuilder<> &builder,
											 Function *&llvmFunc,
											 astNode *node, map<string, Value *> &allocaMap,
											 map<string, Function *> &functionMap,
											 Instruction *&top,
											 bool load = true)
{
	assert(node);
	switch (node->type)
	{
	case ast_stmt:
	{
		switch (node->stmt.type)
		{
		case ast_call:
		{
			astCall call = node->stmt.call;
			string name = string{call.name};
			Function *callFunc = functionMap[ARM_FUNC_PRE+name];
			std::vector<Value *> args = {};
			if (call.params)
			{
				for (astNode *param : *call.params)
				{
					args.push_back(parseExpression(builder, llvmFunc, param, allocaMap, functionMap, top));
				}
			}
			return builder.CreateCall(callFunc, args);
		}
		default:
			break;
		}
		break;
	}
	case ast_bexpr:
	{
		Value *lhs = parseExpression(builder, llvmFunc, node->bexpr.lhs, allocaMap, functionMap, top);
		Value *rhs = parseExpression(builder, llvmFunc, node->bexpr.rhs, allocaMap, functionMap, top);
		Instruction::BinaryOps op;

		switch (node->bexpr.op)
		{
		case add:
			op = Instruction::Add;
			break;
		case sub:
			op = Instruction::Sub;
			break;
		case mul:
			op = Instruction::Mul;
			break;
		case divide:
			op = Instruction::SDiv;
			break;
		default:
			break;
		}
		return builder.CreateBinOp(op, lhs, rhs);
	}
	case ast_uexpr:
	{
		Value *uexpr = parseExpression(builder, llvmFunc, node->uexpr.expr, allocaMap, functionMap, top);
		op_type op = node->uexpr.op;
		switch (op)
		{
		case uminus:
		{
			return builder.CreateNeg(uexpr);
		}
		default:
			break;
		}
	}
	case ast_cnst:
	{
		if (node->cnst.type == char_ty)
		{
			return builder.getInt8(node->cnst.value);
		}
		else
		{
			return builder.getInt32(node->cnst.value);
		}
	}
	case ast_var:
	{
		if (node->var.declared)
		{
			allocaMap[string{node->var.name}] = builder.CreateAlloca(getType(node->var.type, builder), 0, nullptr, string{node->var.name});
			if (allocaMap[string{node->var.name}])
			{
				top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
				moveTop(top, ((Instruction *)allocaMap[string{node->var.name}]));
			}
		}
		if (!load)
		{
			return allocaMap[string{node->var.name}];
		}
		Value *allocaP = allocaMap[string{node->var.name}];
		return builder.CreateLoad(getType(node->var.type, builder), allocaP, false);
	}
	case ast_rexpr:
	{
		Value *lhs = parseExpression(builder, llvmFunc, node->rexpr.lhs, allocaMap, functionMap, top);
		Value *rhs = parseExpression(builder, llvmFunc, node->rexpr.rhs, allocaMap, functionMap, top);
		switch (node->rexpr.op)
		{
		case lt:
			return builder.CreateICmpSLT(lhs, rhs, "");
		case gt:
			return builder.CreateICmpSGT(lhs, rhs, "");
			break;
		case le:
			return builder.CreateICmpSLE(lhs, rhs, "");
		case ge:
			return builder.CreateICmpSGE(lhs, rhs, "");
		case eq:
			return builder.CreateICmpEQ(lhs, rhs, "");
		case neq:
			return builder.CreateICmpNE(lhs, rhs, "");
		default:
			break;
		}
	}
	default:
		break;
	}
	return nullptr;
}

void generateIR(astNode *iNode, string input, string output)
{
	if (iNode->type != ast_prog)
	{
		cerr << "Wrong type or astNode. Must begin with an ast_prog" << endl;
		exit(1);
	}
	InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  string err;
  const string targetTriple = sys::getDefaultTargetTriple();
	LLVMContext context;
	Module *module = new Module("Prog", context);
	module->setTargetTriple(targetTriple);
	map<string, Function *> functionMap;
	IRBuilder<> builder(context);
	Instruction *top = nullptr;
	astProg prog = iNode->prog;
	astFunc func = prog.func->func;
	Type *retType = getType(func.type, builder);

	std::vector<Type *> argTypes;
	if (func.params != nullptr)
	{
		for (astNode *p : *func.params)
		{
			argTypes.push_back(getType(p->var.type, builder));
		}
	}
	FunctionType *funcType = FunctionType::get(retType, argTypes, false);
	Function *llvmFunc = Function::Create(funcType, Function::ExternalLinkage, ARM_FUNC_PRE+string{func.name}, module);
	functionMap[ARM_FUNC_PRE+string{func.name}] = llvmFunc;
	vector<astNode *> nodeStack{func.body};
	for (astNode *ext : *prog.exts)
	{
		nodeStack.push_back(ext);
	}
	vector<vector<astNode *>> blockStack;
	map<BasicBlock *, tuple<BasicBlock *, BasicBlock *>> blockSuccessionMap;
	map<string, Value *> allocaMap;
	Value *returnP = nullptr;
	BasicBlock *retBlock = nullptr;

	while (nodeStack.size() || blockStack.size())
	{

		if (!nodeStack.empty())
		{
			astNode *node = nodeStack.back();
			nodeStack.pop_back();
			if (node == nullptr)
			{
				continue;
			}
			switch (node->type)
			{
			case ast_extern:
			{
				string name = string{node->ext.name};
				std::vector<Type *> extArgTypes;
				for (var_type t : *node->ext.args)
				{
					extArgTypes.push_back(getType(t, builder));
				}
				Type *extRetType = getType(node->ext.type, builder);
				FunctionType *extType = FunctionType::get(extRetType, extArgTypes, false);
				Function *extFunc = Function::Create(extType, Function::ExternalLinkage, ARM_FUNC_PRE+name, module);
				functionMap[ARM_FUNC_PRE+name] = extFunc;
				break;
			}
			case ast_stmt:
				switch (node->stmt.type)
				{
				case ast_call:
					parseExpression(builder, llvmFunc, node, allocaMap, functionMap, top);
					break;
				case ast_block:
				{
					if (!builder.GetInsertBlock())
					{
						BasicBlock *block = BasicBlock::Create(context, "", llvmFunc);
						builder.SetInsertPoint(block);
					}
					else if (blockSuccessionMap.find(builder.GetInsertBlock()) == blockSuccessionMap.end())
					{
						BasicBlock *block = BasicBlock::Create(context, "", llvmFunc);
						builder.CreateBr(block);
						builder.SetInsertPoint(block);
					}
					if (llvmFunc->size() == 1)
					{
						if (func.type != void_ty)
						{
							returnP = builder.CreateAlloca(getType(func.type, builder), 0, nullptr, "");
						}
						if (returnP != nullptr)
						{
							BasicBlock &entryBlock = llvmFunc->getEntryBlock();
							top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
							moveTop(top, ((Instruction *)returnP));
						}
						if (func.params != nullptr)
						{
							Function::arg_iterator funcParamValue = llvmFunc->arg_begin();
							for (astNode *param : *func.params)
							{
								astVar var = param->var;
								if (top == nullptr)
								{
									top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
								}
								Value *allocaP = builder.CreateAlloca(funcParamValue->getType(), 0, nullptr, string{var.name});
								moveTop(top, ((Instruction *)allocaP));
								builder.CreateStore(&*funcParamValue++, allocaP, false);
								allocaMap[string{var.name}] = allocaP;
							}
						}
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

					Value *lhs = parseExpression(builder, llvmFunc, node->stmt.asgn.lhs, allocaMap, functionMap, top, false);
					Value *rhs = parseExpression(builder, llvmFunc, node->stmt.asgn.rhs, allocaMap, functionMap, top);
					builder.CreateStore(rhs, lhs, false);
					break;
				}
				case ast_while:
				{
					// loop condition code
					astWhile whileNode = node->stmt.whilen;
					// loop body basic block and successor
					BasicBlock *cmpBB = BasicBlock::Create(context, "", llvmFunc);
					BasicBlock *loopBlock = BasicBlock::Create(context, "", llvmFunc);
					BasicBlock *finalBlock = BasicBlock::Create(context, "", llvmFunc);
					BasicBlock *prevBlock = builder.GetInsertBlock();
					builder.CreateBr(cmpBB);
					builder.SetInsertPoint(cmpBB);
					Value *cond = parseExpression(builder, llvmFunc, whileNode.cond, allocaMap, functionMap, top);
					blockSuccessionMap[loopBlock] = make_tuple(finalBlock, cmpBB);
					builder.CreateCondBr(cond, loopBlock, finalBlock);
					if (blockSuccessionMap.find(prevBlock) != blockSuccessionMap.end())
					{
						blockSuccessionMap[finalBlock] = blockSuccessionMap[prevBlock];
						blockSuccessionMap.erase(prevBlock);
					}
					builder.SetInsertPoint(loopBlock);
					nodeStack.push_back(whileNode.body);
					break;
				}
				case ast_if:
				{
					astIf ifNode = node->stmt.ifn;
					Value *cond = parseExpression(builder, llvmFunc, ifNode.cond, allocaMap, functionMap, top);
					BasicBlock *finalBlock = nullptr;
					BasicBlock *ifBlock = BasicBlock::Create(context, "", llvmFunc);
					BasicBlock *elseBlock = nullptr;
					if (ifNode.else_body)
					{
						elseBlock = BasicBlock::Create(context, "", llvmFunc);
						finalBlock = BasicBlock::Create(context, "", llvmFunc);
						blockSuccessionMap[ifBlock] = make_tuple(elseBlock, finalBlock);
						blockSuccessionMap[elseBlock] = make_tuple(finalBlock, finalBlock);
						builder.CreateCondBr(cond, ifBlock, elseBlock);
						nodeStack.push_back(ifNode.else_body);
					}
					else
					{
						finalBlock = BasicBlock::Create(context, "", llvmFunc);
						blockSuccessionMap[ifBlock] = make_tuple(finalBlock, finalBlock);
						builder.CreateCondBr(cond, ifBlock, finalBlock);
					}
					if (blockSuccessionMap.find(builder.GetInsertBlock()) != blockSuccessionMap.end())
					{
						blockSuccessionMap[finalBlock] = blockSuccessionMap[builder.GetInsertBlock()];
						blockSuccessionMap.erase(builder.GetInsertBlock());
					}
					nodeStack.push_back(ifNode.if_body);
					builder.SetInsertPoint(ifBlock);
				}
				break;
				case ast_ret:
				{
					Value *retexpr = parseExpression(builder, llvmFunc, node->stmt.ret.expr, allocaMap, functionMap, top);
					if (blockSuccessionMap.find(builder.GetInsertBlock()) != blockSuccessionMap.end())
					{
						if (!retBlock)
						{
							retBlock = BasicBlock::Create(context, "", llvmFunc);
						}
						builder.CreateStore(retexpr, returnP, false);
						tuple<BasicBlock *, BasicBlock *> nextBr = blockSuccessionMap[builder.GetInsertBlock()];
						blockSuccessionMap[builder.GetInsertBlock()] = make_tuple(get<0>(nextBr), retBlock);
					}
					else
					{
						if (!retBlock)
						{
							builder.CreateRet(retexpr);
							// ((Instruction *)returnP)->eraseFromParent();
						}
						else
						{
							builder.CreateStore(retexpr, returnP, false);
							blockSuccessionMap[builder.GetInsertBlock()] = make_tuple(retBlock, retBlock);
						}
					}
					break;
				}
				case ast_decl:
				{
					string varName{node->stmt.decl.name};
					Value *allocaP = builder.CreateAlloca(getType(node->stmt.decl.type, builder), nullptr, varName);
					if (top == nullptr)
					{
						top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
					}
					moveTop(top, ((Instruction *)allocaP));
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
		}
		else
		{
			nodeStack = blockStack.back();
			blockStack.pop_back();
			if (blockSuccessionMap.find(builder.GetInsertBlock()) != blockSuccessionMap.end())
			{
				tuple<BasicBlock *, BasicBlock *> nextBr = blockSuccessionMap[builder.GetInsertBlock()];
				builder.CreateBr(get<1>(nextBr));
				builder.SetInsertPoint(get<0>(nextBr));
			}
		}
	}
	if (returnP->getNumUses() == 0)
	{
		dyn_cast<Instruction>(returnP)->eraseFromParent();
	}

	if (retBlock)
	{
		if (retBlock->getNumUses() == 0)
		{
			retBlock->eraseFromParent();
			retBlock = nullptr;
		}
		else
		{
			builder.CreateBr(retBlock);
			builder.SetInsertPoint(retBlock);
			builder.CreateRet(builder.CreateLoad(getType(func.type, builder), returnP, false));
		}
	}
	else if (func.type == void_ty)
	{
		builder.CreateRetVoid();
	}

	vector<BasicBlock *> toErase;
	for (auto &block : *llvmFunc)
	{
		if (!block.hasNPredecessorsOrMore(1) && &block != &(llvmFunc->getEntryBlock()))
		{
			toErase.push_back(&block);
		}
	}
	// remove redundant blocks
	for (auto &block : toErase)
	{
		block->eraseFromParent();
	}

	// optimize module
	optimizeModule(*module);

	// write optimized ll file
	ofstream ofs(output + ".ll");
	raw_os_ostream rosf(ofs);
	module->print(rosf, nullptr);

	codeGen(*module, input, output + ".s");
}
