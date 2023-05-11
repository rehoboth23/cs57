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

#ifndef _OPT_H_
#define _opt_h_

#define prt(x)         \
  if (x)               \
  {                    \
    printf("%s\n", x); \
  }

#include <cstddef>
#include <string>
#include <iostream>
#include <llvm-c/Types.h>
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Transforms/Scalar.h>

void eliminateCommonSubExpression(LLVMBasicBlockRef bb, bool *change);
void eliminateDeadCode(LLVMBasicBlockRef bb, bool *change);
void constantFolding(LLVMBasicBlockRef bb, bool *change);
void constantPropagation(LLVMValueRef function, bool *change);
void saveModule(LLVMModuleRef module, const char *filename);
#endif
