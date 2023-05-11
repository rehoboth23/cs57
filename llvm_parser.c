#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <iostream>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include "opt.h"

#define prt(x)         \
	if (x)               \
	{                    \
		printf("%s\n", x); \
	}

LLVMModuleRef createLLVMModel(char *filename)
{
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL)
	{
		prt(err);
		return NULL;
	}

	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL)
	{
		prt(err);
	}

	return m;
}

void walkBasicblocks(LLVMValueRef function)
{
	bool change;
	do
	{
		change = false;
		for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function);
				 basicBlock;
				 basicBlock = LLVMGetNextBasicBlock(basicBlock))
		{

			eliminateCommonSubExpression(basicBlock, &change);
			eliminateDeadCode(basicBlock, &change);
			constantFolding(basicBlock, &change);
			constantPropagation(function, &change);
		}
	} while (change);
}

void walkFunctions(LLVMModuleRef module)
{
	for (LLVMValueRef function = LLVMGetFirstFunction(module);
			 function;
			 function = LLVMGetNextFunction(function))
	{

		const char *funcName = LLVMGetValueName(function);

		// printf("\nFunction Name: %s\n", funcName);

		walkBasicblocks(function);
	}
}

void walkGlobalValues(LLVMModuleRef module)
{
	for (LLVMValueRef gVal = LLVMGetFirstGlobal(module);
			 gVal;
			 gVal = LLVMGetNextGlobal(gVal))
	{

		const char *gName = LLVMGetValueName(gVal);
		printf("Global variable name: %s\n", gName);
	}
}

int main(int argc, char **argv)
{
	LLVMModuleRef m;
	if (argc >= 2)
	{
		m = createLLVMModel(argv[1]);
	}
	else
	{
		m = NULL;
		return 1;
	}

	if (m != NULL)
	{
		walkFunctions(m);
		if (argc == 3)
		{
			saveModule(m, argv[2]);
		}
		else
		{
			saveModule(m, NULL);
		}
	}
	else
	{
		printf("m is NULL\n");
	}
	return 0;
}
