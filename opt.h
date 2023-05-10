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

#include <cstddef>
#include <llvm-c/Types.h>

void eliminateCommonSubExpression(LLVMBasicBlockRef bb);

void eliminateDeadCode(LLVMBasicBlockRef bb);
void constantFolding(LLVMBasicBlockRef bb);
#endif
