#include "code_generation.h"
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "extras.h"

// ********************* FUNCTION DECLARATIONS ********************* //


// ********************* GLOBAL VARIABLES ********************* //


// ********************* MAIN FUNCTION DEFINITIONS ********************* //
/*
    writeAssemblyCode
*/
FILE* writeAssemblyCode(LLVMModuleRef optimizedLLVMModule, char* outputAssemblyFileName, char* input_c_file_name) {
    // generate the register allocation for the module
    unordered_map<LLVMValueRef, unordered_map<LLVMValueRef, int>> moduleRegisterAllocationMap = allocateRegistersToModule(optimizedLLVMModule);

    // open the output file for writing
    FILE* outputFile = fopen(outputAssemblyFileName, "w");

    // for each function in the module
    for (LLVMValueRef function = LLVMGetFirstFunction(optimizedLLVMModule); function != NULL; function = LLVMGetNextFunction(function)){
        // get the name of the function 
        size_t length;
        const char* functionName = LLVMGetValueName2(function, &length);

        // skip read and print functions
        if (strcmp(functionName, (const char*)"read") == 0 || strcmp(functionName, (const char*)"print") == 0){
            continue;
        }

        // print the directives to the file
        printDirectives(outputFile, EMIT_FUNCTION_DIRECTIVE, input_c_file_name, functionName);

        // print the function name to the file 
        fprintf(outputFile, "%s:\n", functionName);
        
        // get the offset and local memory map for the function
        tuple<unordered_map<LLVMValueRef, int>, int> offsetInfo = getOffsetMap(function, moduleRegisterAllocationMap[function]);
        unordered_map<LLVMValueRef, int> offsetMap = get<0>(offsetInfo);
        int localMem = get<1>(offsetInfo);

        // create the basic block labels
        unordered_map<LLVMBasicBlockRef, char*> basicBlockLabels = createBasicBlockLabels(function);

        // for each basic block in the function
        int i = 0;
        for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock != NULL; basicBlock = LLVMGetNextBasicBlock(basicBlock)){
            // print the label of the basic block to the file
            fprintf(outputFile, ".%s:\n", basicBlockLabels[basicBlock]);

            // print the name of the basic block
            // size_t length;
            // printf("Basic Block Name: %s\n", LLVMGetBasicBlockName(basicBlock));
            // // print the label for the basic block and all the insructions in it to stdout
            // printf("Basic Block Label: %s\n", basicBlockLabels[basicBlock]);
            // for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
            //     LLVMDumpValue(instruction);
            // }
            // printf("\n\n");

            // print the directives to the file if it is the first basic block
            if (i == 0){
                printDirectives(outputFile, PUSH_CALLER_EBP_UPDATE_EBP_TO_ESP, input_c_file_name, functionName);

                // allocate space for the local variables
                if (abs(localMem) > 0){
                    fprintf(outputFile, "\tsubl $%d, %%esp\t# Allocate Space for Local Variables\n", abs(localMem)); 
                }
            }
            i++;

            // for each instruction in the basic block
            for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)) {
                // print the instruction
                //printf("Instruction: %s\n", LLVMPrintValueToString(instruction));


                // RETURN
                // if the instruction is a return 
                if (LLVMGetInstructionOpcode(instruction) == LLVMRet){
                    // get the operand of the instruction
                    LLVMValueRef operand = LLVMGetOperand(instruction, 0);

                    // if the operand is a constant
                    if (LLVMIsAConstantInt(operand)){
                        // get the value of the constant
                        int value = LLVMConstIntGetSExtValue(operand);

                        // print the instruction to the file
                        fprintf(outputFile, "\tmovl $%d, %%eax\n", value);
                    } else {
                        // if the operand is a variable that is in memory 
                        if (moduleRegisterAllocationMap[function][operand] == SPILL){
                            // get the offset of the variable
                            int offset = offsetMap[operand];

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %d(%%ebp), %%eax\n", offset);
                        }

                        // if the operand is variable that has a register allocated to it 
                        if (moduleRegisterAllocationMap[function].count(operand) > 0 && moduleRegisterAllocationMap[function][operand] != SPILL){
                            // get the register allocated to the operand
                            int registerAllocated = moduleRegisterAllocationMap[function][operand];

                            // get the register name
                            char* registerName = getRegisterName(registerAllocated);

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %%%s, %%eax\n", registerName);
                        }
                    }

                    // print the function end to the file
                    printFunctionEnd(outputFile);
                }

                // LOAD
                // if the instruction is a load
                if (LLVMGetInstructionOpcode(instruction) == LLVMLoad){
                    // if the result is assigned to a physical register
                    if (moduleRegisterAllocationMap[function].count(instruction) > 0 && moduleRegisterAllocationMap[function][instruction] != SPILL) {
                        // get the operand of the instruction
                        LLVMValueRef operand = LLVMGetOperand(instruction, 0);

                        // get the offset for the instruction
                        int offsetOperand = offsetMap[operand];

                        // get the register allocated to the instruction
                        int registerAllocated = moduleRegisterAllocationMap[function][instruction];

                        // get the register name
                        char* registerName = getRegisterName(registerAllocated);
                        
                        // print the offset and the register name 
                        //printf("Offset: %d, Register Name: %s\n", offsetOperand, registerName);

                        // print the instruction to the file
                        fprintf(outputFile, "\tmovl %d(%%ebp), %%%s\n", offsetOperand, registerName);
                    }
                }

                // STORE
                // if the instruction is a store
                if (LLVMGetInstructionOpcode(instruction) == LLVMStore){
                    // get the operands of the instruction
                    LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
                    LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);

                    // print the instructions and the operands 
                    // printf("\tInstruction: %s\n", LLVMPrintValueToString(instruction));
                    // printf("\tOperand 1: %s\n", LLVMPrintValueToString(operand1));
                    // printf("\tOperand 2: %s\n", LLVMPrintValueToString(operand2));

                    // if the first operand is a parameter
                    // store i32 %2, ptr %a, align 4
                    if (LLVMGetValueKind(operand1) == LLVMArgumentValueKind) {
                        continue; // ignore
                    }

                    // if the first operand is a constant
                    if (LLVMIsAConstantInt(operand1)){
                        // get the value of the constant
                        int value = LLVMConstIntGetSExtValue(operand1);

                        // get the offset of the second operand
                        int offset = offsetMap[operand2];

                        // print the instruction to the file
                        fprintf(outputFile, "\tmovl $%d, %d(%%ebp)\n", value, offset);
                    } else {
                        // the first operand is a temporary variable
                        // check if it has a physical register allocated to it
                        if (moduleRegisterAllocationMap[function].count(operand1) > 0 && moduleRegisterAllocationMap[function][operand1] != SPILL){
                            // get the register allocated to the operand
                            int registerAllocated = moduleRegisterAllocationMap[function][operand1];

                            // get the register name
                            char* registerName = getRegisterName(registerAllocated);

                            // get the offset of the second operand
                            int offset = offsetMap[operand2];

                            // print the LLVM instruction
                            // printf("Instruction: %s\n", LLVMPrintValueToString(instruction));
                            // printf("\tOffset: %d, Register Name: %s\n", offset, registerName);

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %%%s, %d(%%ebp)\n", registerName, offset);
                        } else {
                            // get the offset of the first operand
                            int offset = offsetMap[operand1];

                            // get the offset of the second operand
                            int offset2 = offsetMap[operand2];

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %d(%%ebp), %%eax\n", offset);
                            fprintf(outputFile, "\tmovl %%eax, %d(%%ebp)\n", offset2);
                        }
                    }
                }

                // CALL
                // if the instruction is a call
                if (LLVMGetInstructionOpcode(instruction) == LLVMCall){
                    // print instructions to the file 
                    fprintf(outputFile, "\tpushl %%ebx\n");
                    fprintf(outputFile, "\tpushl %%ecx\n");
                    fprintf(outputFile, "\tpushl %%edx\n");
                    
                    // if the function has a parameter 
                    unsigned numParams = LLVMGetNumArgOperands(instruction);
                    if (numParams > 0) {
                        // we expect only one parameter in miniC
                        // thus numParams = 1
                        // if the parameter is a constant 
                        LLVMValueRef param = LLVMGetOperand(instruction, 0);
                        if (LLVMIsAConstantInt(param)){
                            // get the value of the constant
                            int value = LLVMConstIntGetSExtValue(param);

                            // print the instruction to the file
                            fprintf(outputFile, "\tpushl $%d\n", value);
                        } else {
                            // that is the parameter is a temporary variable
                            // check if it has a physical register allocated to it
                            if (moduleRegisterAllocationMap[function].count(param) > 0 && moduleRegisterAllocationMap[function][param] != SPILL){
                                // get the register allocated to the operand
                                int registerAllocated = moduleRegisterAllocationMap[function][param];

                                // get the register name
                                char* registerName = getRegisterName(registerAllocated);

                                // print the instruction to the file
                                fprintf(outputFile, "\tpushl %%%s\n", registerName);
                            } else { // the parameter is a variable that is in memory
                                // get the offset of the parameter
                                int offset = offsetMap[param];

                                // print the instruction to the file
                                fprintf(outputFile, "\tpushl %d(%%ebp)\n", offset);
                            }
                        }
                    }

                    // print the call instruction to the file
                    size_t length2;
                    fprintf(outputFile, "\tcalll %s\n", LLVMGetValueName2(LLVMGetCalledValue(instruction), &length2));

                    // if the function has a parameter
                    if (numParams > 0) {
                        // pop the parameter from the stack
                        fprintf(outputFile, "\taddl $4, %%esp\n");
                    }

                    // print instructions to the file
                    fprintf(outputFile, "\tpopl %%edx\n");
                    fprintf(outputFile, "\tpopl %%ecx\n");
                    fprintf(outputFile, "\tpopl %%ebx\n");

                    // if the instruction returns a value
                    if (LLVMGetTypeKind(LLVMTypeOf(instruction)) != LLVMVoidTypeKind){
                        // check if the instruction has a register allocated to it
                        if (moduleRegisterAllocationMap[function].count(instruction) > 0 && moduleRegisterAllocationMap[function][instruction] != SPILL){
                            // get the register allocated to the instruction
                            int registerAllocated = moduleRegisterAllocationMap[function][instruction];

                            // get the register name
                            char* registerName = getRegisterName(registerAllocated);

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %%eax, %%%s\n", registerName);
                        } else {
                            // get the offset of the instruction
                            int offset = offsetMap[instruction];

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %%eax, %d(%%ebp)\n", offset);
                        }
                    }
                }

                // BRANCH INSTRUCTIONS
                // if the instruction is a branch instruction
                if (LLVMGetInstructionOpcode(instruction) == LLVMBr) {
                    unsigned numOperands = LLVMGetNumOperands(instruction);
                    // if the branch is unconditional 
                    if (numOperands == 1) {
                        // get the operand of the instruction
                        LLVMValueRef operand = LLVMGetOperand(instruction, 0);

                        // get the label of the basic block
                        char* label = basicBlockLabels[LLVMValueAsBasicBlock(operand)];

                        // print the instruction to the file
                        fprintf(outputFile, "\tjmp .%s\n", label);
                    } else if (numOperands == 3) {
                        // get the basic blocks that the instruction branches to
                        // print the instruction
                        printf("Instruction: %s\n", LLVMPrintValueToString(instruction));

                        // print the operands for the instruction
                        printf("Operand 1: %s\n", LLVMPrintValueToString(LLVMGetOperand(instruction, 0)));
                        printf("Operand 2: %s\n", LLVMPrintValueToString(LLVMGetOperand(instruction, 1)));
                        printf("Operand 3: %s\n", LLVMPrintValueToString(LLVMGetOperand(instruction, 2)));

                        // get the basic blocks that the instruction branches to
                        LLVMBasicBlockRef basicBlock1 = LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 1));
                        LLVMBasicBlockRef basicBlock2 = LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 2));

                        // print the basic blocks
                        printf("Basic Block 1: %s\n", LLVMGetBasicBlockName(basicBlock1));
                        printf("Basic Block 2: %s\n", LLVMGetBasicBlockName(basicBlock2));

                        // // print out all the instructions in the basic blocks
                        printf("Basic Block 1 Instructions:\n");
                        LLVMBasicBlockRef basicBlock1Iterator = basicBlock1;
                        while (basicBlock1Iterator != NULL) {
                            LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock1Iterator);
                            while (instruction != NULL) {
                                printf("%s\n", LLVMPrintValueToString(instruction));
                                instruction = LLVMGetNextInstruction(instruction);
                            }
                            basicBlock1Iterator = LLVMGetNextBasicBlock(basicBlock1Iterator);
                        }

                        printf("Basic Block 2 Instructions:\n");
                        LLVMBasicBlockRef basicBlock2Iterator = basicBlock2;
                        while (basicBlock2Iterator != NULL) {
                            LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock2Iterator);
                            while (instruction != NULL) {
                                printf("%s\n", LLVMPrintValueToString(instruction));
                                instruction = LLVMGetNextInstruction(instruction);
                            }
                            basicBlock2Iterator = LLVMGetNextBasicBlock(basicBlock2Iterator);
                        }

                        // get the labels of the basic blocks
                        char* label1 = basicBlockLabels[basicBlock1];
                        char* label2 = basicBlockLabels[basicBlock2];

                        // print the basic block labels
                        printf("Label 1: %s\n", label1);
                        printf("Label 2: %s\n", label2);


                        // get the operand of the instruction
                        LLVMValueRef operand = LLVMGetOperand(instruction, 0);

                        // get the predicate of the instruction
                        LLVMIntPredicate predicate = LLVMGetICmpPredicate(operand);

                        // TODO: Review this code with Prof. Vasanta
                        // write the appropriate instruction to the file
                        // if (predicate == LLVMIntEQ) {
                        //     fprintf(outputFile, "\tje .%s\n", label1);
                        // } else if (predicate == LLVMIntNE) {
                        //     fprintf(outputFile, "\tjne .%s\n", label1);
                        // } else if (predicate == LLVMIntSGT) {
                        //     fprintf(outputFile, "\tjg .%s\n", label1);
                        // } else if (predicate == LLVMIntSGE) {
                        //     fprintf(outputFile, "\tjge .%s\n", label1);
                        // } else if (predicate == LLVMIntSLT) {
                        //     fprintf(outputFile, "\tjl .%s\n", label1);
                        // } else if (predicate == LLVMIntSLE) {
                        //     fprintf(outputFile, "\tjle .%s\n", label1);
                        // }

                        // print the instruction to the file
                        // fprintf(outputFile, "\tjmp .%s\n", label2);

                        // write the appropriate instruction to the file
                        if (predicate == LLVMIntEQ) {
                            fprintf(outputFile, "\tje .%s\n", label2);
                        } else if (predicate == LLVMIntNE) {
                            fprintf(outputFile, "\tjne .%s\n", label2);
                        } else if (predicate == LLVMIntSGT) {
                            fprintf(outputFile, "\tjg .%s\n", label2);
                        } else if (predicate == LLVMIntSGE) {
                            fprintf(outputFile, "\tjge .%s\n", label2);
                        } else if (predicate == LLVMIntSLT) {
                            fprintf(outputFile, "\tjl .%s\n", label2);
                        } else if (predicate == LLVMIntSLE) {
                            fprintf(outputFile, "\tjle .%s\n", label2);
                        }

                        // print the instruction to the file
                        fprintf(outputFile, "\tjmp .%s\n", label1);
                    }
                }

                // ALLOCA
                // if the instruction is an alloca
                if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca){
                    continue; // ignore
                }

                // ADD/CMP/SUB/MUL
                // arithmetic instructions
                LLVMOpcode instOpcode = LLVMGetInstructionOpcode(instruction);
                if (instOpcode == LLVMAdd || instOpcode == LLVMSub || instOpcode == LLVMMul || instOpcode == LLVMICmp) {
                    // print the instruction 
                    //printf("\tReading instruction: %s\n", LLVMPrintValueToString(instruction));
                    
                    // check if the result is assigned to a physical register
                    int x;
                    int inMemory = 0;
                    if (moduleRegisterAllocationMap[function].count(instruction) > 0 && moduleRegisterAllocationMap[function][instruction] != SPILL){
                        x = moduleRegisterAllocationMap[function][instruction];
                        //printf("\t\tInstruction has a register, x = %s\n", getRegisterName(x));
                    } else {
                        x = EAX;
                        inMemory = 1;
                        //printf("\t\tInstruction does not have a register, taking EAX\n");
                    }

                    // get the operands
                    LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
                    LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);

                    // if the first operand is a constant
                    if (LLVMIsAConstantInt(operand1)){
                        // get the value of the constant
                        int value = LLVMConstIntGetSExtValue(operand1);

                        // print the instruction to the file
                        fprintf(outputFile, "\tmovl $%d, %%%s\n", value, getRegisterName(x));
                    } else { // the operand is a variable
                        // check if the operand has a register allocated to it
                        if (moduleRegisterAllocationMap[function].count(operand1) > 0 && moduleRegisterAllocationMap[function][operand1] != SPILL){
                            // get the register allocated to the operand
                            int registerAllocated = moduleRegisterAllocationMap[function][operand1];

                            // get the register name
                            char* registerName = getRegisterName(registerAllocated);

                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %%%s, %%%s\n", registerName, getRegisterName(x));
                        } else { // the operand is a variable that is in memory
                            // get the offset of the operand
                            int offset = offsetMap[operand1];
                                
                            // print the instruction to the file
                            fprintf(outputFile, "\tmovl %d(%%ebp), %%%s\n", offset, getRegisterName(x));
                        }
                    }

                    // if the second operand is a constant
                    if (LLVMIsAConstantInt(operand2)){
                        // get the value of the constant
                        int value = LLVMConstIntGetSExtValue(operand2);

                        // based on the opcode i.e. the operation, print the instruction to the file
                        if (instOpcode == LLVMAdd) {
                            fprintf(outputFile, "\taddl $%d, %%%s\n", value, getRegisterName(x));
                        } else if (instOpcode == LLVMSub) {
                            fprintf(outputFile, "\tsubl $%d, %%%s\n", value, getRegisterName(x));
                        } else if (instOpcode == LLVMMul) {
                            fprintf(outputFile, "\timull $%d, %%%s\n", value, getRegisterName(x));
                        } else if (instOpcode == LLVMICmp) {
                            fprintf(outputFile, "\tcmpl $%d, %%%s\n", value, getRegisterName(x));
                        }
                    } else { // the operand is a variable
                        // check if the operand has a register allocated to it
                        if (moduleRegisterAllocationMap[function].count(operand2) > 0 && moduleRegisterAllocationMap[function][operand2] != SPILL){
                            // get the register allocated to the operand
                            int registerAllocated = moduleRegisterAllocationMap[function][operand2];

                            // get the register name
                            char* registerName = getRegisterName(registerAllocated);

                            // print the instruction, the registerName and the register in x
                            // printf("\nIn here\n");
                            // printf("Instruction: %s\n", LLVMPrintValueToString(instruction));
                            // printf("\tmovl %%%s, %%%s\n", registerName, getRegisterName(x));
                            // printf("Done\n\n");

                            // based on the opcode i.e. the operation, print the instruction to the file
                            if (instOpcode == LLVMAdd) {
                                fprintf(outputFile, "\taddl %%%s, %%%s\n", registerName, getRegisterName(x));
                            } else if (instOpcode == LLVMSub) {
                                fprintf(outputFile, "\tsubl %%%s, %%%s\n", registerName, getRegisterName(x));
                            } else if (instOpcode == LLVMMul) {
                                fprintf(outputFile, "\timull %%%s, %%%s\n", registerName, getRegisterName(x));
                            } else if (instOpcode == LLVMICmp) {
                                fprintf(outputFile, "\tcmpl %%%s, %%%s\n", registerName, getRegisterName(x));
                            }
                        } else {
                            // get the offset of the operand
                            int offset = offsetMap[operand2];

                            // based on the opcode i.e. the operation, print the instruction to the file
                            if (instOpcode == LLVMAdd) {
                                fprintf(outputFile, "\taddl %d(%%ebp), %%%s\n", offset, getRegisterName(x));
                            } else if (instOpcode == LLVMSub) {
                                fprintf(outputFile, "\tsubl %d(%%ebp), %%%s\n", offset, getRegisterName(x));
                            } else if (instOpcode == LLVMMul) {
                                fprintf(outputFile, "\timull %d(%%ebp), %%%s\n", offset, getRegisterName(x));
                            } else if (instOpcode == LLVMICmp) {
                                fprintf(outputFile, "\tcmpl %d(%%ebp), %%%s\n", offset, getRegisterName(x));
                            }
                        }
                    }
                    if (inMemory) {
                        // get the offset of the result 
                        int instructionOffset = offsetMap[instruction];

                        // print the instruction to the file 
                        fprintf(outputFile, "\tmovl %%eax, %d(%%ebp)\n", instructionOffset);
                    }
                }
            }
            // print a new line to the file
            fprintf(outputFile, "\n");
        }

        // clean up the basic block labels
        for (auto basicBlock : basicBlockLabels){
            free(basicBlock.second);
        }
    }

    return outputFile;
}


/*
    computeRegisterAllocation
*/
unordered_map<LLVMValueRef, unordered_map<LLVMValueRef, int>>
    allocateRegistersToModule(LLVMModuleRef optimizedLLVMModule) {
        // initialize the map
        unordered_map<LLVMValueRef, unordered_map<LLVMValueRef, int>> moduleRegisterAllocationMap;

        // iterate over the functions in the module
        for (LLVMValueRef function = LLVMGetFirstFunction(optimizedLLVMModule); function != NULL; function = LLVMGetNextFunction(function)){
            // print the function name
            // printf("Function: %s\n", LLVMPrintValueToString(function));

            // compute the live variables and live ranges for each basic block in the function
            tuple<unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>>, 
                unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, pair<int, int>>>> liveness = computeLiveness(function);


            // for each basic block, sort the instructions by the end of their live range
            unordered_map<LLVMBasicBlockRef, vector<pair<LLVMValueRef, int>>> sortedInstructions;
            for (auto basicBlock : get<1>(liveness)){
                sortedInstructions[basicBlock.first] = sortInstructionsInBasicBlock(basicBlock.second);
            }

            // print the sorted instructions
            // printf("Sorted Instructions:\n");
            // for (auto basicBlock : sortedInstructions){
            //     printf("\tBasic Block: \n");
            //     for (auto instruction : basicBlock.second){
            //         printf("\t\tInstruction: %s, End: %d\n", LLVMPrintValueToString(instruction.first), instruction.second);
            //     }
            // }

            // compute the register allocation for the function 
            unordered_map<LLVMValueRef, int> registerAllocation;
            registerAllocation = computeRegisterAllocation(
                function, get<1>(liveness), 
                get<0>(liveness),
                sortedInstructions);

            // add the register allocation for the function
            moduleRegisterAllocationMap[function] = registerAllocation;

            // print the register allocation for the function
            // printf("Register Allocation:\n");
            // for (auto basicBlock : registerAllocation){
            //     printf("\tInstruction: %s, Register: %d\n", LLVMPrintValueToString(basicBlock.first), basicBlock.second);
            // }

            // generate the basic block labels
            unordered_map<LLVMBasicBlockRef, char*> basicBlockLabels = createBasicBlockLabels(function);


            // TODO: clean up basic block labels

            // printf("Finished Function\n\n");
        }
        
        // return the map
        return moduleRegisterAllocationMap;
    }


// ********************* HELPER FUNCTION DEFINITIONS ********************* //
/*
    computeLiveness
*/
tuple<unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>>, 
    unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, pair<int, int>>>>
    computeLiveness(LLVMValueRef function){
        // initialize the maps
        unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> instructionIndex;
        unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, pair<int, int>>> liveRanges;

        // compute instruction index for each basic block 
        for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock != NULL; basicBlock = LLVMGetNextBasicBlock(basicBlock)){
            // initialize the map for the basic block
            unordered_map<LLVMValueRef, int> instructionIndexForBasicBlock;

            // initialize the index
            int index = 0;

            // iterate over the instructions in the basic block
            for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)){
                // ignore the alloca instructions
                if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca){
                    continue;
                }

                // add the instruction to the map
                instructionIndexForBasicBlock[instruction] = index;

                // initialize the live range for the instruction
                    // that is their range is fixed to where they are initialized
                liveRanges[basicBlock][instruction] = make_pair(index, index);

                // increment the index
                index++;
            }
            // add the instruction to the map
            instructionIndex[basicBlock] = instructionIndexForBasicBlock;
        }

        // compute live ranges for each basic block
        for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock != NULL; basicBlock = LLVMGetNextBasicBlock(basicBlock)){
            // iterate over the instructions in the basic block
            for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)){
                // ignore the alloca instructions
                if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca){
                    continue;
                }

                // ignore instructions that do not produce a result
                // if (LLVMGetTypeKind(LLVMTypeOf(instruction)) == LLVMVoidTypeKind){
                //     continue;
                // }
            
                // print the instruction 
                // printf("Instruction: %s\n", LLVMPrintValueToString(instruction));

                // iterate over the operands of the instruction
                for (int i = 0; i < LLVMGetNumOperands(instruction); i++){
                    // get the operand
                    LLVMValueRef operand = LLVMGetOperand(instruction, i);

                    // print the operand 
                    // printf("\tOperand: %s\n", LLVMPrintValueToString(operand));

                    // ignore the operands that are not instructions
                    if (LLVMIsAInstruction(operand) == NULL){
                        continue;
                    }

                    // ignore the operands that are not in the same basic block
                    if (LLVMGetInstructionParent(operand) != basicBlock){
                        continue;
                    }

                    // get the index of the instruction
                    int instIndex = instructionIndex[basicBlock][instruction];

                    // update the live range of the instruction
                    // if it is in the map
                    if (liveRanges[basicBlock].count(operand) > 0){
                        liveRanges[basicBlock][operand].second = instIndex;
                    }
                }
            }
        }

        // print the instruction index
        printf("Instruction Index:\n");
        for (auto basicBlock : instructionIndex){
            printf("\tBasic Block: \n");
            for (auto instruction : basicBlock.second){
                printf("\t\tInstruction: %s, Index: %d\n", LLVMPrintValueToString(instruction.first), instruction.second);
            }
        }
        printf("\n\n");

        // // print the live ranges
        printf("Live Ranges:\n");
        for (auto basicBlock : liveRanges){
            printf("\tBasic Block: \n");
            for (auto instruction : basicBlock.second){
                printf("\t\tInstruction: %s, Start: %d, End: %d\n", LLVMPrintValueToString(instruction.first), instruction.second.first, instruction.second.second);
            }
        }
        printf("\n\n");

        // return the computed maps 
        return make_tuple(instructionIndex, liveRanges);
    }

/*
    computeRegisterAllocation
*/
unordered_map<LLVMValueRef, int>
    computeRegisterAllocation(LLVMValueRef function,
        unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, pair<int, int>>> liveRanges,
        unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> instructionIndex,
        unordered_map<LLVMBasicBlockRef, vector<pair<LLVMValueRef, int>>> sortedInstructionsMap
        ) {
            // initialize the map
                // use one single map since we single static assignment
                // thus instructions are not redefined over basic blocks
            unordered_map<LLVMValueRef, int> functionRegisterAllocationMap;
            // printf("Computing Register Allocation: \n");

            // iterate over the basic blocks in the function
            for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock != NULL; basicBlock = LLVMGetNextBasicBlock(basicBlock)){
                // get the live ranges for the basic block
                unordered_map<LLVMValueRef, pair<int, int>> liveRangesForBasicBlock = liveRanges[basicBlock];

                // get tne instruction index for the basic block
                unordered_map<LLVMValueRef, int> instructionIndexForBasicBlock = instructionIndex[basicBlock];

                // get the sorted instructions for the basic block
                vector<pair<LLVMValueRef, int>> sortedInstructionsForBasicBlock = sortedInstructionsMap[basicBlock];

                // initialize a set of available registers 
                set<int> availableRegisters = {EBX, ECX, EDX};

                // iterate over the instructions in the basic block
                for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)){
                    // ignore the alloca instructions
                    if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca){
                        continue;
                    }

                    // HANDLE INSTRUCTIONS THAT DO NOT RETURN A VALUE
                    if (LLVMGetTypeKind(LLVMTypeOf(instruction)) == LLVMVoidTypeKind){
                        LLVMValueRef possibleEndOfLiveRangeOperand = 
                            checkIfLiveRangeEnds(liveRangesForBasicBlock, instruction, instructionIndexForBasicBlock[instruction], functionRegisterAllocationMap);
                        if (possibleEndOfLiveRangeOperand != NULL){
                            availableRegisters.insert(functionRegisterAllocationMap[possibleEndOfLiveRangeOperand]);
                        }
                        continue;
                    }

                    // HANDLE ADD/SUB/MUL INSTRUCTIONS
                    // get the first operand
                    LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
                    LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction); 
                    if ((opcode == LLVMOpcode::LLVMAdd || opcode == LLVMOpcode::LLVMSub || opcode == LLVMOpcode::LLVMMul) 
                    && (functionRegisterAllocationMap.count(operand1) > 0) 
                    && (liveRangesForBasicBlock[operand1].second == instructionIndexForBasicBlock[instruction])) {
                        // print the instruction
                        // printf("\t\tInstruction (add/mul/sub): %s\n", LLVMPrintValueToString(instruction));
                        // get the register allocated to the operand
                        // and assign it to the instruction
                        functionRegisterAllocationMap[instruction] = functionRegisterAllocationMap[operand1];

                        // print the register allocated to the instruction
                        // printf("\tThe register allocated to the instruction is %s\n", getRegisterName(functionRegisterAllocationMap[instruction]));

                        // print the register allocated to the operand
                        // printf("\tThe register allocated to the operand is %s\n", getRegisterName(functionRegisterAllocationMap[operand1]));
                        
                        // check if the liveness range of the second operand ends at the current instruction
                        LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);

                        // print the second operand 
                        // printf("The second operands is %s\n", LLVMPrintValueToString(operand2));

                        // print the register for the second operand
                        // printf("\tThe register allocated to the second operand is %s\n", getRegisterName(functionRegisterAllocationMap[operand2]));

                        if (liveRangesForBasicBlock[operand2].second == instructionIndexForBasicBlock[instruction]){
                            // check if the second operand has a register allocated to it
                            if (functionRegisterAllocationMap.count(operand2) > 0){
                                // since the liveness of the second operand ends
                                // add the register to the pool of available registers
                                availableRegisters.insert(functionRegisterAllocationMap[operand2]);
                            }
                        }
                        continue;
                    }

                    // IF A PHYSICAL REGISTER IS AVAILABLE
                    if (availableRegisters.size() > 0){
                        // allocate the register to the instruction
                        functionRegisterAllocationMap[instruction] = *availableRegisters.begin();

                        // remove the register from the pool of available registers
                        availableRegisters.erase(availableRegisters.begin());

                        LLVMValueRef possibleEndOfLiveRangeOperand = 
                            checkIfLiveRangeEnds(liveRangesForBasicBlock, instruction, instructionIndexForBasicBlock[instruction], functionRegisterAllocationMap);
                        if (possibleEndOfLiveRangeOperand != NULL){
                            availableRegisters.insert(functionRegisterAllocationMap[possibleEndOfLiveRangeOperand]);
                        }
                    }
                    // IF NO PHYSICAL REGISTER IS AVAILABLE
                    else {
                        // call spill function to see if a variable can be spilled
                        LLVMValueRef possibleSpillVariable = findSpill(functionRegisterAllocationMap, sortedInstructionsForBasicBlock);

                        // check if the liveness range of the instruction 
                        // end after liveness range of the possible spill variable
                        if (possibleSpillVariable != NULL) {
                            if (liveRangesForBasicBlock[instruction].second > liveRangesForBasicBlock[possibleSpillVariable].second){
                                // assign no physical register to the instruction
                                functionRegisterAllocationMap[instruction] = SPILL;
                            } else {
                                // get the physical register allocated to the possible spill variable
                                int registerAllocated = functionRegisterAllocationMap[possibleSpillVariable];

                                // assign the register to the instruction
                                functionRegisterAllocationMap[instruction] = registerAllocated;

                                // spill the possible spill variable
                                functionRegisterAllocationMap[possibleSpillVariable] = SPILL;
                            }
                        } else {
                            // assign no physical register to the instruction
                            functionRegisterAllocationMap[instruction] = SPILL;
                        }

                        // check if the live range of any of the operands ends at the current instruction
                        LLVMValueRef possibleEndOfLiveRangeOperand = 
                            checkIfLiveRangeEnds(liveRangesForBasicBlock, instruction, instructionIndexForBasicBlock[instruction], functionRegisterAllocationMap);
                        if (possibleEndOfLiveRangeOperand != NULL){
                            availableRegisters.insert(functionRegisterAllocationMap[possibleEndOfLiveRangeOperand]);
                        }
                    }
                }
            }

            // print the register allocation map
            // printf("Register allocation map for function: \n");
            // for (auto instruction : functionRegisterAllocationMap){
            //     printf("\t%s -> %s\n", LLVMPrintValueToString(instruction.first), getRegisterName(instruction.second));
            // }


            // return the map
            return functionRegisterAllocationMap;
        }

/*
    sortInstructionsInBasicBlock
*/
vector<pair<LLVMValueRef, int>>
    sortInstructionsInBasicBlock(unordered_map<LLVMValueRef, pair<int, int>> liveRangeForBasicBlock){
        // create a map of instruction to the end of its live range 
        unordered_map<LLVMValueRef, int> instructionToLiveRangeEnd;

        // populate the map
        for (auto instruction : liveRangeForBasicBlock){
            instructionToLiveRangeEnd[instruction.first] = instruction.second.second;
        }

        // initialize the vector 
        vector<pair<LLVMValueRef, int>> sortedInstructions(instructionToLiveRangeEnd.begin(), instructionToLiveRangeEnd.end());

        // sort the instructions by the end of their live range
        sort(sortedInstructions.begin(), sortedInstructions.end(), 
            [](const pair<LLVMValueRef, int> &a, const pair<LLVMValueRef, int> &b) -> bool { 
                return a.second > b.second; 
            });
        
        // print the sorted instructions
        // printf("Sorted Instructions:\n");
        // for (auto instruction : sortedInstructions){
        //     printf("\tInstruction: %s, End: %d\n", LLVMPrintValueToString(instruction.first), instruction.second);
        // }

        // return the vector
        return sortedInstructions;
    }

/*
    findSpill
*/
LLVMValueRef findSpill(unordered_map<LLVMValueRef, int> registerMap,
    vector<pair<LLVMValueRef, int>> sortedInstructions
    ) {
        // initialize the spill variable
        LLVMValueRef spillVariable = NULL;

        // iterate over the instructions in the basic block
        for (auto instruction : sortedInstructions){
            // check if the instruction has a register allocated to it
            if (registerMap.count(instruction.first) > 0){
                // get the register allocated to the instruction
                int registerAllocated = registerMap[instruction.first];

                // check if the register is not the spill register
                if (registerAllocated != SPILL){
                    // set the spill variable
                    spillVariable = instruction.first;

                    // break out of the loop
                    break;
                }
            }
        }

        // return the spill variable
        return spillVariable;
    }

/*
    checkIfLiveRangeEnds
*/
LLVMValueRef checkIfLiveRangeEnds(
    unordered_map<LLVMValueRef, pair<int, int>> liveRangeForBasicBlock,
    LLVMValueRef instruction,
    int instructionIndex,
    unordered_map<LLVMValueRef, int> registerMap
    ) {
        // iterate through the operands for the instruction
        for (int i = 0; i < LLVMGetNumOperands(instruction); i++){
            // get the operand
            LLVMValueRef operand = LLVMGetOperand(instruction, i);

            // check if the operand has a register allocated to it
            if (registerMap.count(operand) > 0){
                // check if the liveness range of the operand 
                // ends at the current instruction
                if (liveRangeForBasicBlock[operand].second == instructionIndex){
                    // since the liveness of the operand ends
                    // return the register allocated to the operand
                    return operand;
                }
            }
            
        }
        // return NULL if no operand has a live range that ends at the current instruction
        return NULL;
    }

/*
    createBasicBlockLabels
*/
unordered_map<LLVMBasicBlockRef, char*> createBasicBlockLabels(LLVMValueRef function) {
    // initialize the map
    unordered_map<LLVMBasicBlockRef, char*> basicBlockLabels;

    // iterate over the basic blocks in the function
    int count = 1;
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock != NULL; basicBlock = LLVMGetNextBasicBlock(basicBlock)){
        // create a label for the basic block
        char* label = (char*)malloc(sizeof(char) * 100);
        sprintf(label, "BasicBlock_%d", count++);

        // add the label to the map
        basicBlockLabels[basicBlock] = label;
    }

    // print the labels
    // printf("Basic Block Labels:\n");
    // for (auto basicBlock : basicBlockLabels){
    //     printf("\tBasic Block Label: %s\n", basicBlock.second);
    // }

    // return the map
    return basicBlockLabels;
}


/*
    getOffsetMap
*/
tuple<unordered_map<LLVMValueRef, int>, int> getOffsetMap(LLVMValueRef function, unordered_map<LLVMValueRef, int> registerMap) {
    // initialize the map
    unordered_map<LLVMValueRef, int> offsetMap;

    // initialize localMem 
    int localMem = 0;

    // print the register map
    printf("Register Map:\n");
    for (auto registerAllocation : registerMap){
        printf("\tInstruction: %s, Register: %s\n", LLVMPrintValueToString(registerAllocation.first), getRegisterName(registerAllocation.second));
    }

    // iterate over the basic blocks in the function
    for (LLVMBasicBlockRef basicBlock = LLVMGetFirstBasicBlock(function); basicBlock != NULL; basicBlock = LLVMGetNextBasicBlock(basicBlock)){
        // iterate over the instructions in the basic block
        for (LLVMValueRef instruction = LLVMGetFirstInstruction(basicBlock); instruction != NULL; instruction = LLVMGetNextInstruction(instruction)){
            //printf("\t\t\t [Computing Offset] Instruction: %s\n", LLVMPrintValueToString(instruction));
           
            // if the instruction is an alloca instruction
            if (LLVMGetInstructionOpcode(instruction) == LLVMAlloca){
                // subtract the size from the local memory
                localMem -= 4; 

                // add the instruction to the offset map
                offsetMap[instruction] = localMem;
                continue;
            }

            // if the instruction is a store instruction
            if (LLVMGetInstructionOpcode(instruction) == LLVMStore) {
                // get the operands of the instruction
                LLVMValueRef operand1 = LLVMGetOperand(instruction, 0);
                LLVMValueRef operand2 = LLVMGetOperand(instruction, 1);

                // if the first operand is the parameter for the function
                if (LLVMGetValueKind(operand1) == LLVMArgumentValueKind){
                    // since it is a parameter, the offset is 8
                    // thus we update the offset of the second operand
                    offsetMap[operand2] = 8;

                    // since we have changed an offset, we need to update the local memory 
                    // and everything in the offset map
                    // for (auto offset : offsetMap){
                    //     // if the key is not operand2
                    //     if (offset.first != operand2){
                    //         // update the offset
                    //         offsetMap[offset.first] += 4;
                    //     }
                    // }
                    // localMem += 4;
                }
                continue; 
            }

            // if the instruction returns a value and the instruction is not assigned to a register
            if (LLVMGetTypeKind(LLVMTypeOf(instruction)) != LLVMVoidTypeKind && 
                (registerMap.count(instruction) > 0 && registerMap[instruction] == SPILL)){
                // if it is a load instruction 
                if (LLVMGetInstructionOpcode(instruction) == LLVMLoad){
                    // get the first operand of the instruction
                    LLVMValueRef operand = LLVMGetOperand(instruction, 0);

                    // if the operand is a parameter
                    if (LLVMGetValueKind(operand) == LLVMArgumentValueKind){
                        // since it is a parameter, the offset is 8
                        // thus we update the offset of the instruction
                        offsetMap[instruction] = 8;
                    } else {
                        // the operand is a local variable 
                        // get the offset of the operand
                        int offset = offsetMap[operand];

                        // set the offset of the instruction
                        offsetMap[instruction] = offset;
                    }
                    continue;
                }

                // for other instructions
                // iterate over the uses of the instruction 
                //printf("\tIn offset map computation\n");
                //printf("\t\tInstruction: %s\n", LLVMPrintValueToString(instruction));
                for (LLVMUseRef use = LLVMGetFirstUse(instruction); use != NULL; use = LLVMGetNextUse(use)){
                    // get the instruction 
                    LLVMValueRef useInstruction = LLVMGetUser(use);
                    //printf("\t\tUse Instruction: %s\n", LLVMPrintValueToString(useInstruction));

                    // if the use is a store instruction
                    if (LLVMGetInstructionOpcode(useInstruction) == LLVMStore) {
                        // get the second operand of the instruction (i.e. the variable where the value is stored)
                        LLVMValueRef operand2 = LLVMGetOperand(useInstruction, 1);

                        // print the operand
                        //printf("\t\tOperand2: %s\n", LLVMPrintValueToString(operand2));

                        // set the offset of the instruction
                        offsetMap[instruction] = offsetMap[operand2];
                        break; 
                    }
                }
            }
        }
    }

    // TODO: crosscheck offset map with Vasanta
    // print the offset map
    printf("Offset Map:\n");
    for (auto instruction : offsetMap){
        printf("\tInstruction: %s, Offset: %d\n", LLVMPrintValueToString(instruction.first), instruction.second);
    }

    // return the map and the local memory
    return make_tuple(offsetMap, localMem);
}

/*
    printDirectives
*/
void printDirectives(FILE* assemblyFile, functionDirectives directive, char* input_c_file_name, const char* function_name) {
    if (directive == EMIT_FUNCTION_DIRECTIVE){
        fprintf(assemblyFile, "\t.file \"%s\"\n", input_c_file_name); // print the file name
        fprintf(assemblyFile, "\t.text\n"); // indicate that the following code is text
        fprintf(assemblyFile, "\t.globl %s\n", function_name); // indicate that the function is global
        fprintf(assemblyFile, "\t.type %s, @function\n", function_name); // indicate that the function is a function
    } else if (directive == PUSH_CALLER_EBP_UPDATE_EBP_TO_ESP){
        // push %ebp
        fprintf(assemblyFile, "\tpushl %%ebp        # Save the old base pointer value.\n");

        // movl %esp, %ebp
        fprintf(assemblyFile, "\tmovl %%esp, %%ebp   # Set the new base pointer value.\n");
    }
}

/*
    printFunctionEnd
*/
void printFunctionEnd(FILE* assemblyFile) {
    // print to restore the stack pointer
    fprintf(assemblyFile, "\tmovl %%ebp, %%esp   # Restore stack pointer\n");

    // restore the base pointer
    fprintf(assemblyFile, "\tpopl %%ebp         # Restore base pointer\n");

    // return to the caller
    fprintf(assemblyFile, "\tret               # Return to caller\n");
}

/*
    getRegisterName
*/
char* getRegisterName(int registerNumber) {
    switch (registerNumber){
        case EAX: 
            return (char*)"eax";
        case EBX:
            return (char*)"ebx";
        case ECX:
            return (char*)"ecx";
        case EDX:
            return (char*)"edx";
        case SPILL:
            return (char*)"SPILL";
        default:
            return (char*)"";
    }
}