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

#include "codegen.h"

using namespace std;
using namespace llvm;

inline void log(string s)
{
#ifdef LOG
  cout << s << endl;
#endif
}

#ifdef ARMD
map<int, string> regName = {
    {EBX, "r1"},
    {ECX, "r2"},
    {EDX, "r3"},
    {EAX, "r0"},
};
string intLit = "#";
string stackPointer = "r13";
string basePointer = "r14";
string amov = "mov";
string aadd = "add";
string asub = "sub";
string amul = "mul";
string acmp = "cmp";
string apop = "pop";
string apush = "push";
#else
map<int, string> regName = {
    {EBX, "%ebx"},
    {ECX, "%ecx"},
    {EDX, "%edx"},
    {EAX, "%eax"},
};
string intLit = "$";
string stackPointer = "%esp";
string basePointer = "%ebp";
string amov = "movl";
string aadd = "addl";
string asub = "subl";
string amul = "mull";
string acmp = "cmpl";
string apop = "popl";
string apush = "pushl";
#endif

void generateAssemblyCode(Module &module, string asmFileName)
{
  // Get the target triple from the module
  Triple triple(module.getTargetTriple());

  // Get the target machine
  string error;
  const Target *target = TargetRegistry::lookupTarget(triple.getTriple(), error);
  if (!target)
  {
    errs() << "Failed to lookup target: " << error << "\n";
    return;
  }

  // Set target options
  TargetOptions options;
  options.MCOptions.AsmVerbose = true; // Enable verbose assembly output

  // Create the target machine
  TargetMachine *targetMachine = target->createTargetMachine(triple.getTriple(), "", "", options, Optional<Reloc::Model>());
  if (!targetMachine)
  {
    errs() << "Failed to create target machine\n";
    return;
  }
  std::error_code ec;
  llvm::raw_fd_ostream outputFile(asmFileName, ec);
  if (ec) {
    llvm::errs() << "Error opening file: " << ec.message();
  }
  legacy::PassManager passManager = legacy::PassManager();
  targetMachine->addPassesToEmitFile(
      passManager, outputFile, nullptr, CGFT_AssemblyFile);
  passManager.run(module);
  outputFile.flush();
}

Value *findSpill(map<Value *, int> regMap, vector<Value *> &valVec)
{
  for (Value *&val : valVec)
  {
    if (regMap.find(val) != regMap.end() && regMap[val] != -1)
    {
      return val;
    }
  }
  return nullptr;
}

void computeLiveness(BasicBlock &block, map<Value *, int> &instIdx, map<Value *, tuple<int, int>> &liveRange, vector<Value *> &valVec)
{
  /**
   * 1) a map (inst_index), that maps the instructions to their index in the basic-block (first instruction is at index 0). The alloca instructions are ignored while creating the indices.

 2) a map (live_range), that maps the instructions in the given basic block to a pair of integers (start, end). The first integer in the pair indicates the index of the instruction that defines the value and second number gives the index of the last instruction that uses the value in the current basic block.
  */
  int count{0};
  for (Instruction &inst : block)
  {
    valVec.push_back(&inst);
    unsigned opCode = inst.getOpcode();
    Value *op1 = nullptr;
    Value *op2 = nullptr;
    if (inst.getNumOperands() > 0)
    {
      op1 = inst.getOperand(0);
    }
    if (inst.getNumOperands() > 1)
    {
      op2 = inst.getOperand(1);
    }
    instIdx[&inst] = count;
    if (liveRange.find(&inst) == liveRange.end())
    {
      liveRange[&inst] = make_tuple(count, count);
    }
    if (op1 != nullptr && liveRange.find(op1) != liveRange.end())
    {
      get<1>(liveRange[op1]) = count;
    }
    if (op2 != nullptr && liveRange.find(op2) != liveRange.end())
    {
      get<1>(liveRange[op2]) = count;
    }
    count += 1;
  }
}

void freeRegister(
    Value *op,
    Value *inst,
    set<unsigned> &availablePhysRegs,
    map<Value *, tuple<int, int>> liveRange,
    map<Value *, int> instIdx,
    map<Value *, int> regMap)
{
  if (op != nullptr && instIdx[inst] >= get<1>(liveRange[op]) && regMap.find(op) != regMap.end())
  {
    availablePhysRegs.insert(regMap[op]);
  }
}

void createBBLabels(Function &function, map<Value *, string> &bbLabels)
{
  int count = 1;
  for (BasicBlock &block : function)
  {
    bbLabels[&block] = "b" + to_string(count++);
  }
}

void printDirectives(ofstream &asmFileStream, functionDirectives directive, string inputFileName, string functionName)
{
  if (directive == EMIT_FUNCTION_DIRECTIVE)
  {
    asmFileStream << "\t.file \"" << inputFileName << "\"" << endl
                  << "\t.text" << endl
                  << "\t.globl " << functionName << endl
                  << "\t.type " << functionName << ", @function" << endl;
  }
  else if (directive == PUSH_CALLER_EBP_UPDATE_EBP_TO_ESP)
  {
    asmFileStream << "\t" << apush << " " << basePointer << endl
                  << "\t" << amov << " " << stackPointer << ", " << basePointer << endl;
  }
}

void printFunctionEnd(ofstream &asmFileStream)
{
  asmFileStream << "\t" << amov << " " << basePointer << ", " << stackPointer << endl
                << "\t" << apop << " " << basePointer << endl
                << "\tret" << endl;
}

int getOffsetMap(Function &function, map<Value *, int> &offsetMap, map<Value *, int> regMap)
{
  int numArgs = function.arg_size();
  int offset = numArgs * 4;
  for (BasicBlock &block : function)
  {
    for (Instruction &inst : block)
    {
      unsigned opCode = inst.getOpcode();
      Value *op1 = nullptr;
      Value *op2 = nullptr;
      if (inst.getNumOperands() > 0)
      {
        op1 = inst.getOperand(0);
      }
      if (inst.getNumOperands() > 1)
      {
        op2 = inst.getOperand(1);
      }
      switch (opCode)
      {
      case Instruction::Alloca:
      {
        Type *valueType = ((Value *)&inst)->getType();
        offset -= 4;
        offsetMap[&inst] = offset;
        break;
      }
      case Instruction::Store:
        if (isa<Argument>(op1))
        {
          offsetMap[op2] = 8;
        }
        break;
      default:
        if (
            inst.getType()->isVoidTy() &&
            regMap.find(&inst) != regMap.end() &&
            regMap[&inst] == -1)
        {
          if (opCode == Instruction::Load)
          {
            if (isa<Argument>(op1))
            {
              offsetMap[&inst] = 8;
            }
            else
            {
              offsetMap[&inst] = offsetMap[op1];
            }
          }
          else
          {
            for (Value *user : inst.users())
            {
              Instruction *useInst = dyn_cast<Instruction>(user);
              if (useInst->getOpcode() == Instruction::Store)
              {
                Value *userOp2 = useInst->getOperand(1);
                offsetMap[&inst] = offsetMap[userOp2];
                break;
              }
            }
          }
        }
        break;
      }
    }
  }
  return offset;
}

string getRegisterName(int reg)
{
  if (regName.find(reg) != regName.end())
  {
    return regName[reg];
  }

  return "";
}

void writeAsmFile(
    Module &module,
    string inputFileName,
    string asmFile,
    map<Value *, map<Value *, int>> moduleRegMap)
{
  ofstream asmFileStream(asmFile);
  for (Function &func : module)
  {
    map<Value *, int> regMap = moduleRegMap[&func];
    if (!func.isDeclaration())
    {
      printDirectives(asmFileStream, EMIT_FUNCTION_DIRECTIVE, inputFileName, func.getName().str());
      asmFileStream << func.getName().str() << ":" << endl;
      map<Value *, int> offsetMap;
      map<Value *, string> bbLabels;
      int offset = getOffsetMap(func, offsetMap, regMap);
      createBBLabels(func, bbLabels);
      int count = 0;
      for (BasicBlock &block : func)
      {
        asmFileStream << "." << bbLabels[&block] << ":" << endl;
        if (count == 0)
        {
          printDirectives(asmFileStream, PUSH_CALLER_EBP_UPDATE_EBP_TO_ESP, inputFileName, func.getName().str());
          if (offset != 0)
          {
            asmFileStream << "\t" << asub << " " << intLit << abs(offset) << ", " << stackPointer << "\t" << endl;
          }
        }
        count++;
        for (Instruction &inst : block)
        {
          string instructionString;
          raw_string_ostream instructionStream(instructionString);
          inst.print(instructionStream);
          log("# " + instructionString);

          unsigned opCode = inst.getOpcode();
          Value *op1 = nullptr;
          Value *op2 = nullptr;
          if (inst.getNumOperands() > 0)
          {
            op1 = inst.getOperand(0);
          }
          if (inst.getNumOperands() > 1)
          {
            op2 = inst.getOperand(1);
          }
          if (opCode == Instruction::Ret && op1 && !op1->getType()->isVoidTy())
          {
            ConstantInt *constOp = dyn_cast<ConstantInt>(op1);
            if (constOp != nullptr)
            {
              asmFileStream << "\t" << amov << " " << intLit << constOp->getSExtValue() << ", " << getRegisterName(EAX) << endl;
            }
            else
            {
              if (regMap.find(op1) != regMap.end() && regMap[op1] == -1)
              {
                int opOffset = offsetMap[op1];
                asmFileStream << "\t" << amov << " " << opOffset << "(" << basePointer << "), " << getRegisterName(EAX) << endl;
              }
              else if (regMap[op1] != -1)
              {
                string reg = getRegisterName(regMap[op1]);
                asmFileStream << "\t" << amov << " " << reg << ", " << getRegisterName(EAX) << endl;
              }
            }
            printFunctionEnd(asmFileStream);
          }
          else if (opCode == Instruction::Load && regMap.find(&inst) != regMap.end())
          {
            string reg = getRegisterName(regMap[&inst]);
            int opOffset = offsetMap[op1];
            asmFileStream << "\t" << amov << " " << opOffset << "(" << basePointer << "), " << getRegisterName(EAX) << endl;
          }
          else if (opCode == Instruction::Store)
          {
            if (!isa<Argument>(op1))
            {
              ConstantInt *constOp = dyn_cast<ConstantInt>(op1);
              if (constOp != nullptr)
              {
                int opOffset = offsetMap[op2];
                asmFileStream << "\t" << amov << " " << intLit << constOp->getSExtValue() << ", " << opOffset << "(" << basePointer << ")" << endl;
              }
              else if (regMap.find(op1) != regMap.end() && regMap[op1] != -1)
              {

                string reg = getRegisterName(regMap[op1]);
                int opOffset = offsetMap[op2];
                asmFileStream << "\t" << amov << " " << opOffset << "(" << basePointer << "), " << reg << endl;
              }
              else
              {
                int opOffset1 = offsetMap[op1];
                int opOffset2 = offsetMap[op2];
                asmFileStream << "\t" << amov << " " << opOffset1 << "(" << basePointer << "), " << getRegisterName(EAX) << endl;
                asmFileStream << "\t" << amov << " " << getRegisterName(EAX) << ", " << opOffset2 << "(" << basePointer << ")" << endl;
              }
            }
          }
          else if (opCode == Instruction::Call)
          {
            asmFileStream << "\t" << apush << " " << getRegisterName(EBX) << "" << endl;
            asmFileStream << "\t" << apush << " " << getRegisterName(ECX) << "" << endl;
            asmFileStream << "\t" << apush << " " << getRegisterName(EDX) << "" << endl;
            const CallInst *callInst = dyn_cast<CallInst>(&inst);
            Function *calledFunction = callInst->getCalledFunction();
            Type *returnType = calledFunction->getReturnType();
            for (int argIdx = 0; argIdx < inst.getNumOperands(); argIdx++)
            {
              Value *op = inst.getOperand(argIdx);
              ConstantInt *constOp = dyn_cast<ConstantInt>(op);
              if (constOp != nullptr)
              {
                asmFileStream << "\t" << apush << " " << intLit << constOp->getSExtValue() << endl;
              }
              else if (regMap.find(op) != regMap.end() && regMap[op] != -1)
              {
                string reg = getRegisterName(regMap[op1]);
                asmFileStream << "\t" << apush << " " << reg << endl;
              }
              else
              {
                int opOffset = offsetMap[op];
                asmFileStream << "\t" << apush << " " << opOffset << "(" << basePointer << ")" << endl;
              }
            }
            asmFileStream << "\tcall " << calledFunction->getName().str() << endl;
            for (int argIdx = 0; argIdx < inst.getNumOperands(); argIdx++)
            {
              asmFileStream << "\t" << aadd << " $4, " << stackPointer << endl;
            }
            asmFileStream << "\t" << apop << " " << getRegisterName(EDX) << "" << endl
                          << "\t" << apop << " " << getRegisterName(ECX) << "" << endl
                          << "\t" << apop << " " << getRegisterName(EBX) << "" << endl;
            if (!returnType->isVoidTy())
            {
              if (regMap.find(&inst) != regMap.end() && regMap[&inst] != -1)
              {
                string reg = getRegisterName(regMap[&inst]);
                asmFileStream << "\t" << amov << " " << getRegisterName(EAX) << ", " << reg << endl;
              }
              else
              {
                int opOffset = offsetMap[&inst];
                asmFileStream << "\t" << amov << " " << getRegisterName(EAX) << ", " << opOffset << "(" << basePointer << ")\n"
                              << endl;
              }
            }
          }
          else if (opCode == Instruction::Br)
          {
            Value *op3 = nullptr;
            if (inst.getNumOperands() == 1)
            {
              asmFileStream << "\tjmp ." << bbLabels[op1] << endl;
            }
            else if (inst.getNumOperands() >= 3)
            {
              op3 = inst.getOperand(2);
              CmpInst::Predicate predicate = cast<ICmpInst>(op1)->getPredicate();
              switch (predicate)
              {
              case CmpInst::ICMP_SGT:
                asmFileStream << "jg ." << bbLabels[op2] << endl;
                break;
              case CmpInst::ICMP_SLT:
                asmFileStream << "jl ." << bbLabels[op2] << endl;
                break;
              case CmpInst::ICMP_SGE:
                asmFileStream << "jge ." << bbLabels[op2] << endl;
                break;
              case CmpInst::ICMP_SLE:
                asmFileStream << "jle ." << bbLabels[op2] << endl;
                break;
              case CmpInst::ICMP_EQ:
                asmFileStream << "je ." << bbLabels[op2] << endl;
                break;
              case CmpInst::ICMP_NE:
                asmFileStream << "jne ." << bbLabels[op2] << endl;
                break;
              default:
                break;
              }
              asmFileStream << "jmp ." << bbLabels[op3] << endl;
            }
          }
          else if (opCode == Instruction::Add ||
                   opCode == Instruction::Sub ||
                   opCode == Instruction::Mul ||
                   opCode == Instruction::ICmp)
          {
            int instReg = EAX;
            bool inMemory = true;
            if (regMap.find(&inst) != regMap.end() && regMap[&inst] != -1)
            {
              instReg = regMap[&inst];
              inMemory = false;
            }

            ConstantInt *constOp = dyn_cast<ConstantInt>(op1);
            if (constOp != nullptr)
            {
              asmFileStream << "\t" << amov << " " << intLit << constOp->getSExtValue() << ", " << getRegisterName(instReg) << endl;
            }
            else if (regMap.find(op1) != regMap.end() && regMap[op1] != -1)
            {
              asmFileStream << "\t" << amov << " " << getRegisterName(regMap[op1]) << ", " << getRegisterName(instReg) << endl;
            }
            else
            {
              asmFileStream << "\t" << amov << " " << offsetMap[op1] << "(" << basePointer << "), " << getRegisterName(instReg) << endl;
            }
            constOp = dyn_cast<ConstantInt>(op2);
            if (constOp != nullptr)
            {

              asmFileStream << "\t" << (opCode == Instruction::Add ? aadd : (opCode == Instruction::Sub ? asub : (opCode == Instruction::Mul ? amul : (acmp))))
                            << " " << intLit << constOp->getSExtValue() << ", " << getRegisterName(instReg) << endl;
            }
            else if (regMap.find(op2) != regMap.end() && regMap[op2] != -1)
            {
              asmFileStream << "\t" << (opCode == Instruction::Add ? aadd : (opCode == Instruction::Sub ? asub : (opCode == Instruction::Mul ? amul : (acmp))))
                            << " " << getRegisterName(regMap[op2]) << ", " << getRegisterName(instReg) << endl;
            }
            else
            {
              asmFileStream << "\t" << (opCode == Instruction::Add ? aadd : (opCode == Instruction::Sub ? asub : (opCode == Instruction::Mul ? amul : (acmp))))
                            << " " << offsetMap[op2] << "(" << basePointer << "), " << getRegisterName(instReg) << endl;
            }
            if (inMemory)
            {
              int instOffset = offsetMap[&inst];
              asmFileStream << "\t" << amov << " " << getRegisterName(EAX) << ", " << instOffset << "(" << basePointer << ")\n"
                            << endl;
            }
          }
        }
        asmFileStream << endl;
      }
    }
  }
}

void codeGen(Module &module, string inputFileName, string asmFile)
{
  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmParsers();
  InitializeAllAsmPrinters();
  string err;
  const string targetTriple = sys::getDefaultTargetTriple();
  module.setTargetTriple(targetTriple);
#ifndef GEND
  generateAssemblyCode(module, asmFile);
  return;
#endif
  map<Value *, map<Value *, int>> moduleRegMap;
  for (Function &function : module)
  {
    map<Value *, int> regMap;
    // Set the machine function in the MachineRegisterInfo
    for (BasicBlock &block : function)
    {
      set<unsigned> availablePhysRegs = {
          EBX,
          ECX,
          EDX};
      map<Value *, int> instIdx;
      map<Value *, tuple<int, int>> liveRange;
      vector<Value *> valVec;
      computeLiveness(block, instIdx, liveRange, valVec);
      auto sortValVec = [&liveRange](Value *a, Value *b) -> bool
      {
        int aEnd{-1};
        int bEnd{-1};
        if (liveRange.find(a) != liveRange.end())
        {
          aEnd = get<1>(liveRange[a]);
        }
        if (liveRange.find(b) != liveRange.end())
        {
          bEnd = get<1>(liveRange[b]);
        }
        return aEnd > bEnd;
      };
      std::sort(valVec.begin(), valVec.end(), sortValVec);
      for (Instruction &inst : block)
      {
        unsigned opCode = inst.getOpcode();
        Value *op1 = nullptr;
        Value *op2 = nullptr;
        if (inst.getNumOperands() > 0)
        {
          op1 = inst.getOperand(0);
        }
        if (inst.getNumOperands() > 1)
        {
          op2 = inst.getOperand(1);
        }
        switch (opCode)
        {
        case Instruction::Alloca:
          break;
        case Instruction::Store:
        case Instruction::Br:
          freeRegister(op1, &inst, availablePhysRegs, liveRange, instIdx, regMap);
          freeRegister(op2, &inst, availablePhysRegs, liveRange, instIdx, regMap);
          break;
        case Instruction::Call:
        {
          const CallInst *callInst = dyn_cast<CallInst>(&inst);
          if (callInst != nullptr)
          {
            Function *calledFunction = callInst->getCalledFunction();
            if (calledFunction && !calledFunction->isDeclaration())
            {
              Type *returnType = calledFunction->getReturnType();
              if (returnType->isVoidTy())
              {
                break;
              }
            }
          }
        }
        default:
          if ((opCode == Instruction::Add ||
               opCode == Instruction::Sub ||
               opCode == Instruction::Mul) &&
              regMap.find(op1) != regMap.end() &&
              liveRange.find(op1) != liveRange.end() &&
              get<1>(liveRange[op1]) == instIdx[&inst])
          {
            regMap[&inst] = regMap[op1];
            freeRegister(op2, &inst, availablePhysRegs, liveRange, instIdx, regMap);
          }
          else if (availablePhysRegs.size() > 0)
          {
            int physReg = *availablePhysRegs.begin();
            regMap[&inst] = physReg;
            availablePhysRegs.erase(physReg);
            freeRegister(op1, &inst, availablePhysRegs, liveRange, instIdx, regMap);
            freeRegister(op2, &inst, availablePhysRegs, liveRange, instIdx, regMap);
          }
          else
          {
            Value *val = findSpill(regMap, valVec);
            int valEnd{-1};
            if (liveRange.find(val) != liveRange.end())
            {
              valEnd = get<1>(liveRange[val]);
            }
            if (get<1>(liveRange[&inst]) > valEnd)
            {
              regMap[&inst] = -1;
            }
            else
            {
              regMap[&inst] = regMap[val];
              regMap[val] = -1;
            }
            freeRegister(op1, &inst, availablePhysRegs, liveRange, instIdx, regMap);
            freeRegister(op2, &inst, availablePhysRegs, liveRange, instIdx, regMap);
          }
          break;
        }
      }
    }
    moduleRegMap[&function] = regMap;
  }
  writeAsmFile(module, inputFileName, asmFile, moduleRegMap);
}
