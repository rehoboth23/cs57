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
#include "optimizer.h"

using namespace std;

void moveTop(llvm::Instruction *&top, llvm::Instruction *toMove)
{
	toMove->moveAfter(top);
	top = toMove;
}

llvm::Type *getType(var_type type, llvm::IRBuilder<> &builder)
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

llvm::Value *parseExpression(llvm::IRBuilder<> &builder,
														 astNode *node, map<string, llvm::Value *> &allocaMap,
														 map<string, llvm::Function *> &functionMap,
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
			llvm::Function *callFunc = functionMap[name];
			std::vector<llvm::Value *> args = {};
			if (call.params)
			{
				for (astNode *param : *call.params)
				{
					args.push_back(parseExpression(builder, param, allocaMap, functionMap));
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
		llvm::Value *lhs = parseExpression(builder, node->bexpr.lhs, allocaMap, functionMap);
		llvm::Value *rhs = parseExpression(builder, node->bexpr.rhs, allocaMap, functionMap);
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
		llvm::Value *uexpr = parseExpression(builder, node->uexpr.expr, allocaMap, functionMap);
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
			allocaMap[string{node->var.name}] = builder.CreateAlloca(getType(node->var.type, builder), 0, nullptr, "");
		}
		if (!load)
		{
			return allocaMap[string{node->var.name}];
		}
		llvm::Value *allocaP = allocaMap[string{node->var.name}];
		return builder.CreateLoad(getType(node->var.type, builder), allocaP, false);
	}
	case ast_rexpr:
	{
		llvm::Value *lhs = parseExpression(builder, node->rexpr.lhs, allocaMap, functionMap);
		llvm::Value *rhs = parseExpression(builder, node->rexpr.rhs, allocaMap, functionMap);
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

void generateIR(astNode *node, string output)
{
	if (node->type != ast_prog)
	{
		cerr << "Wrong type or astNode. Must begin with an ast_prog" << endl;
		exit(1);
	}
	llvm::LLVMContext context;
	llvm::Module *module = new llvm::Module("Prog", context);
	map<string, llvm::Function *> functionMap;
	llvm::IRBuilder<> builder(context);
	llvm::Instruction *top = nullptr;
	astProg prog = node->prog;
	astFunc func = prog.func->func;
	llvm::Type *retType = getType(func.type, builder);

	std::vector<llvm::Type *> argTypes;
	if (func.params != nullptr)
	{
		for (astNode *p : *func.params)
		{
			argTypes.push_back(getType(p->var.type, builder));
		}
	}
	llvm::FunctionType *funcType = llvm::FunctionType::get(retType, argTypes, false);
	llvm::Function *llvmFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, func.name, module);
	functionMap[string{func.name}] = llvmFunc;
	vector<astNode *> nodeStack{func.body};
	for (astNode *ext : *prog.exts)
	{
		nodeStack.push_back(ext);
	}
	vector<vector<astNode *>> blockStack;
	map<llvm::BasicBlock *, tuple<llvm::BasicBlock *, llvm::BasicBlock *>> blockSuccessionMap;
	map<string, llvm::Value *> allocaMap;
	llvm::Value *returnP = nullptr;
	llvm::BasicBlock *retBlock = nullptr;

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
		case ast_extern:
		{
			string name = string{node->ext.name};
			std::vector<llvm::Type *> extArgTypes;
			for (var_type t : *node->ext.args)
			{
				extArgTypes.push_back(getType(t, builder));
			}
			llvm::Type *extRetType = getType(node->ext.type, builder);
			llvm::FunctionType *extType = llvm::FunctionType::get(extRetType, extArgTypes, false);
			llvm::Function *extFunc = llvm::Function::Create(extType, llvm::Function::ExternalLinkage, name, module);
			functionMap[name] = extFunc;
			break;
		}
		case ast_stmt:
			switch (node->stmt.type)
			{
			case ast_call:
				parseExpression(builder, node, allocaMap, functionMap);
				break;
			case ast_block:
			{
				if (!builder.GetInsertBlock())
				{
					llvm::BasicBlock *block = llvm::BasicBlock::Create(context, "", llvmFunc);
					builder.SetInsertPoint(block);
				}
				else if (blockSuccessionMap.find(builder.GetInsertBlock()) == blockSuccessionMap.end())
				{
					llvm::BasicBlock *block = llvm::BasicBlock::Create(context, "", llvmFunc);
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
						llvm::BasicBlock &entryBlock = llvmFunc->getEntryBlock();
						top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
						moveTop(top, ((llvm::Instruction *)returnP));
					}
					if (func.params != nullptr)
					{
						llvm::Function::arg_iterator funcParamValue = llvmFunc->arg_begin();
						for (astNode *param : *func.params)
						{
							astVar var = param->var;
							if (top == nullptr)
							{
								top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
							}
							llvm::Value *allocaP = builder.CreateAlloca(funcParamValue->getType(), 0, nullptr, string{var.name});
							moveTop(top, ((llvm::Instruction *)allocaP));
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

				llvm::Value *lhs = parseExpression(builder, node->stmt.asgn.lhs, allocaMap, functionMap, false);
				llvm::Value *rhs = parseExpression(builder, node->stmt.asgn.rhs, allocaMap, functionMap);
				builder.CreateStore(rhs, lhs, false);
				break;
			}
			case ast_while:
			{
				// loop condition code
				astWhile whileNode = node->stmt.whilen;
				// loop body basic block and successor
				llvm::BasicBlock *cmpBB = llvm::BasicBlock::Create(context, "", llvmFunc);
				llvm::BasicBlock *loopBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
				llvm::BasicBlock *finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
				llvm::BasicBlock *prevBlock = builder.GetInsertBlock();
				builder.CreateBr(cmpBB);
				builder.SetInsertPoint(cmpBB);
				llvm::Value *cond = parseExpression(builder, whileNode.cond, allocaMap, functionMap);
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
				llvm::Value *cond = parseExpression(builder, ifNode.cond, allocaMap, functionMap);
				llvm::BasicBlock *finalBlock = nullptr;
				llvm::BasicBlock *ifBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
				llvm::BasicBlock *elseBlock = nullptr;
				if (ifNode.else_body)
				{
					elseBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					blockSuccessionMap[ifBlock] = make_tuple(elseBlock, finalBlock);
					blockSuccessionMap[elseBlock] = make_tuple(finalBlock, finalBlock);
					builder.CreateCondBr(cond, ifBlock, elseBlock);
					nodeStack.push_back(ifNode.else_body);
				}
				else
				{
					finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
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
				llvm::Value *retexpr = parseExpression(builder, node->stmt.ret.expr, allocaMap, functionMap);
				if (blockSuccessionMap.find(builder.GetInsertBlock()) != blockSuccessionMap.end())
				{
					if (!retBlock)
					{
						retBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					}
					builder.CreateStore(retexpr, returnP, false);
					tuple<llvm::BasicBlock *, llvm::BasicBlock *> nextBr = blockSuccessionMap[builder.GetInsertBlock()];
					blockSuccessionMap[builder.GetInsertBlock()] = make_tuple(get<0>(nextBr), retBlock);
				}
				else
				{
					if (!retBlock)
					{
						builder.CreateRet(retexpr);
						// ((llvm::Instruction *)returnP)->eraseFromParent();
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
				llvm::Value *allocaP = builder.CreateAlloca(getType(node->stmt.decl.type, builder), nullptr, varName);
				if (top == nullptr)
				{
					top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
				}
				moveTop(top, ((llvm::Instruction *)allocaP));
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
			if (blockSuccessionMap.find(builder.GetInsertBlock()) != blockSuccessionMap.end())
			{
				tuple<llvm::BasicBlock *, llvm::BasicBlock *> nextBr = blockSuccessionMap[builder.GetInsertBlock()];
				builder.CreateBr(get<1>(nextBr));
				builder.SetInsertPoint(get<0>(nextBr));
			}
		}
	}

	if (retBlock)
	{
		builder.CreateBr(retBlock);
		builder.SetInsertPoint(retBlock);
		builder.CreateRet(builder.CreateLoad(getType(func.type, builder), returnP, false));
	}
	else if (func.type == void_ty)
	{
		builder.CreateRetVoid();
	}

	vector<llvm::BasicBlock *> toErase;
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
	ofstream ofs(output);
	llvm::raw_os_ostream rosf(ofs);
	module->print(rosf, nullptr);
	delete module;
}
