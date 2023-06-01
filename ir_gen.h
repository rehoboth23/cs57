/**
 * @file ir_gen.h
 * @author Rehoboth Okorie
 * @brief IR generation
 *
 * @version 0.1
 * @date 2023-05-04
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef IR_GEN_H
#define IR_GEN_H

#include <string>
#include <iostream>
#include "ast.h"
#include "optimizer.h"
#include "codegen.h"

using namespace std;

void generateIR(astNode *iNode, string input, string output);

#endif