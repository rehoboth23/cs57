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

void moveTop(llvm::Instruction*& top, llvm::Instruction *toMove)
{
	toMove->moveAfter(top);
	top = toMove;
}

llvm::Value *parseExpression(llvm::IRBuilder<> &builder, astNode *node, map<string, llvm::Value *> allocaMap, map<string, llvm::Function *> functionMap)
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
			llvm::Function *callFunc = functionMap[call.name];
			if (callFunc)
			{
				std::vector<llvm::Value *> args = {};
				if (call.param)
				{
					args.push_back(parseExpression(builder, call.param, allocaMap, functionMap));
				}
				return builder.CreateCall(callFunc, args);
			}
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
	llvm::Type *retType = nullptr;
	llvm::Instruction *top = nullptr;

	astProg prog = node->prog;
	astFunc func = prog.func->func;

	if (string{func.type} == string{"int"})
	{
		retType = builder.getInt32Ty();
	}
	else
	{
		retType = builder.getVoidTy();
	}
	std::vector<llvm::Type *> argTypes;
	if (func.param != nullptr)
	{
		argTypes.push_back(builder.getInt32Ty());
	}
	llvm::FunctionType *funcType = llvm::FunctionType::get(retType, argTypes, false);
	llvm::Function *llvmFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, func.name, module);
	functionMap[func.name] = llvmFunc;
	vector<astNode *> nodeStack{func.body, prog.ext1, prog.ext2};
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
			if (name == string{"print"})
			{
				extArgTypes.push_back(builder.getInt32Ty());
			}
			llvm::Type *extRetType = name == string{"read"} ? builder.getInt32Ty() : builder.getVoidTy();
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
					if (string{func.type} == string{"int"})
					{
						returnP = builder.CreateAlloca(builder.getInt32Ty(), 0, nullptr, "");
						llvm::BasicBlock &entryBlock = llvmFunc->getEntryBlock();
						top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
						moveTop(top, ((llvm::Instruction *) returnP));
					}
					if (func.param != nullptr)
					{
						astVar var = func.param->var;
						if (top == nullptr) {
							top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
						}
						llvm::Value *allocaP = builder.CreateAlloca(builder.getInt32Ty(), 0, nullptr, string{var.name});
						moveTop(top, ((llvm::Instruction *) allocaP));
						llvm::Value *funcParamValue = llvmFunc->arg_begin();
						builder.CreateStore(funcParamValue, allocaP, false);
						allocaMap[string{var.name}] = allocaP;
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
				llvm::Value *lhs = allocaMap[string{node->stmt.asgn.lhs->var.name}];
				llvm::Value *rhs = parseExpression(builder, node->stmt.asgn.rhs, allocaMap, functionMap);
				builder.CreateStore(rhs, lhs, false);
				break;
			}
			case ast_while:
				break;
			case ast_if:
			{
				astIf ifNode = node->stmt.ifn;
				astRExpr rexpr = ifNode.cond->rexpr;
				llvm::Value *lhs = parseExpression(builder, rexpr.lhs, allocaMap, functionMap);
				llvm::Value *rhs = parseExpression(builder, rexpr.rhs, allocaMap, functionMap);
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
				if (ifNode.else_body)
				{
					elseBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					llvm::BasicBlock *finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					blockSuccessionMap[ifBlock] = make_tuple(elseBlock, finalBlock);
					blockSuccessionMap[elseBlock] = make_tuple(finalBlock, finalBlock);
					builder.CreateCondBr(cond, ifBlock, elseBlock);
					nodeStack.push_back(ifNode.else_body);
				}
				else
				{
					llvm::BasicBlock *finalBlock = llvm::BasicBlock::Create(context, "", llvmFunc);
					blockSuccessionMap[ifBlock] = make_tuple(finalBlock, finalBlock);
					builder.CreateCondBr(cond, ifBlock, finalBlock);
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
				llvm::Value *allocaP = builder.CreateAlloca(builder.getInt32Ty(), nullptr, varName);
				if (top == nullptr)
				{
					top = &*llvmFunc->getEntryBlock().getFirstInsertionPt();
				}
				moveTop(top, ((llvm::Instruction *) allocaP));
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
		builder.CreateRet(builder.CreateLoad(builder.getInt32Ty(), returnP, false));
	}
	else if (string{func.type} == string{"void"})
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
