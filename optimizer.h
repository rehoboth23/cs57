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

#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cassert>
#include "ast.h"

using namespace std;

#define prt(x)         \
  if (x)               \
  {                    \
    printf("%s\n", x); \
  }

void llvmBackendCaller(astNode *node);
#endif
