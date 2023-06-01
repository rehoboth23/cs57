#ifndef  CODE_GENERATION_H
#define CODE_GENERATION_H

#include <llvm-c/Core.h>
#include<unordered_map>
#include<unordered_set>
#include<vector>
#include<tuple>
#include<string>
#include<algorithm> // std::set_union
#include<set>
#include<utility> // std::pair
using namespace std;

// **************** CONSTANTS **************** //
typedef enum {
    /* available registers */
    EBX = 0,
    ECX = 1,
    EDX = 2,
    EAX = 4,

    /* spill register tag */
    SPILL = -1
} registers;

typedef enum {
    /* emit function directives */
    EMIT_FUNCTION_DIRECTIVE = 0,

    /* push caller, %ebp */
    PUSH_CALLER_EBP_UPDATE_EBP_TO_ESP = 1
} functionDirectives;


// **************** MAIN FUNCTIONS **************** //
/*
    Function to generate assembly code from the optimized LLVM module
*/
FILE* writeAssemblyCode(LLVMModuleRef optimizedLLVMModule, char* outputAssemblyFileName, char* input_c_file_name);


/*
    Function to allocate registers for each basic block 
        for each function in the module
    Returns a map:
        key: function
        value: map of basic block to a map of variable to register

*/
unordered_map<LLVMValueRef, unordered_map<LLVMValueRef, int>>
    allocateRegistersToModule(LLVMModuleRef optimizedLLVMModule);


// **************** HELPER FUNCTIONS **************** //
/*
    Helper function to compute the live variables and live ranges
        for each basic block in a function
    Returns a tuple:
        first: map of basic block to a map of an instruction to its index in the basic block
        second: a map of a basic block to a map of a variable to a pair of its start and end index (i.e. live range)
*/
tuple<unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>>, 
    unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, pair<int, int>>>>
    computeLiveness(LLVMValueRef function);

/*
    Helper function to allocate registers to variables
        for each basic block in a function
    Returns a map: 
        key: basic block
        value: map of variable to register
*/
unordered_map<LLVMValueRef, int>
    computeRegisterAllocation(LLVMValueRef function,
        unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, pair<int, int>>> liveRanges,
        unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> instructionIndex,
        unordered_map<LLVMBasicBlockRef, vector<pair<LLVMValueRef, int>>> sortedInstructionsMap
        );

/*
    Function to produce a vector containing the instructions in a basic block 
        sorted by their endpoints in the liveness range
*/
vector<pair<LLVMValueRef, int>> 
    sortInstructionsInBasicBlock(unordered_map<LLVMValueRef, pair<int, int>> liveRangeForBasicBlock);

/*
    Function to find a spill variable in a basic block
        Or return NULL if no variable can be spilled
*/
LLVMValueRef findSpill(unordered_map<LLVMValueRef, int> registerMap,
    vector<pair<LLVMValueRef, int>> sortedInstructions
    );

/*
    Function to check if the live range of any operand of an 
        instruction ends at the instruction
*/
LLVMValueRef checkIfLiveRangeEnds(
    unordered_map<LLVMValueRef, pair<int, int>> liveRangeForBasicBlock,
    LLVMValueRef instruction,
    int instructionIndex,
    unordered_map<LLVMValueRef, int> registerMap
    );

/*
    Function to create labels for basic blocks in a function
*/
unordered_map<LLVMBasicBlockRef, char*> createBasicBlockLabels(LLVMValueRef function);

/*
    Function to populate the global offset map
        Key: LLVMValueRef (an instruction)
            Value: offset (an integer)
    Should also initialize an integer variable 
        localMem to indicate the number of bytes required
        to store the local values 
*/
tuple<unordered_map<LLVMValueRef, int>, int> getOffsetMap(LLVMValueRef function, unordered_map<LLVMValueRef, int> registerMap);

/*
    Function to emit the required directives for 
        the function assembly file
        and push callers %ebp, and update %ebp to current %esp
*/
void printDirectives(FILE* assemblyFile, functionDirectives directive, char* input_c_file_name, const char* function_name);

/*
    Function to emit the assembly instruction to 
        restore the value of %esp and %ebp
*/
void printFunctionEnd(FILE* assemblyFile);

/*
    Function to produce the name of a register based on the number
*/
char* getRegisterName(int registerNumber);

#endif // !CODE_GENERATION_H
