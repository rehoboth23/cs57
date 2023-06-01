/**
 * @file codegen.h
 * @author Rehoboth Okorie
 * @brief A map (reg_map) from LLVMValueRef (a LLVM instruction) to  Assigned physical register.
 * Note that the keys in this map are references to those instructions that generate a value (has a LHS in LLVM IR code).
 * Your register allocation algorithm can use the same map to hold the information computed for each basic block.
 * The map will also contain those values (instructions) that do not have a physical register assigned (spills).
 * The value of assigned physical register for these will be -1 to indicate that they are spilled.
 * Implements the linear-scan register allocation algorithmLinks to an external site. at basic block level.
 *
 * @version 0.1
 * @date 2023-05-04
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _CODE_GEN_
#define _CODE_GEN_

#include <algorithm>
#include "optimizer.h"
#include "llvm/Support/Host.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/Optional.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CodeGen.h"
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>

using namespace std;
using namespace llvm;

enum REG_VALS
{
  EBX = 0,
  ECX = 1,
  EDX = 2,
  EAX = 3,
};

typedef enum
{
  EMIT_FUNCTION_DIRECTIVE = 0,
  PUSH_CALLER_EBP_UPDATE_EBP_TO_ESP = 1
} functionDirectives;

void codeGen(Module &module, string inputFileName, string asmFile);
#endif