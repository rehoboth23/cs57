source = pset
LLVMCODE = optimizer
TEST = p1
OS := $(shell uname -s)
CLANG = clang++ --std=c++2a

ifeq ($(OS), Linux)
LDC=`llvm-config-15 --cflags` -I /usr/include/llvm-c-15/
CDC=`llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/
DEBUGGER=gdb --args
MEM_CHECK=valgrind --leak-check=full --show-leak-kinds=all
else
LDC=`llvm-config --cflags` -I /usr/include/llvm-c/
CDC=`llvm-config --cxxflags --ldflags --libs core` -I /usr/include/llvm-c/
DEBUGGER=lldb --
MEM_CHECK=leaks --fullContent --fullStacks --atExit --
endif

ifeq ($(LOG), LOG)
LOGD=-DLOG
else
LOGD=
endif
ifeq ($(OPTIMIZER), OPTIMIZER)
OPTD=-DOPTIMIZER
else
OPTD=
endif


all: $(LLVMCODE).o $(source).out

.PHONY: all test valgrind debug

$(source).out: $(source).l $(source).y ast.h ast.c sem.h sem.cpp $(LLVMCODE).o
	yacc -d -v -t $(source).y
	lex $(source).l
	$(CLANG) -g $(CDC) $(LLVMCODE).o -o $(source).out lex.yy.c y.tab.c ast.c sem.cpp

# $(LLVMCODE): $(LLVMCODE).o
# 	$(CLANG) $(CDC) $(LLVMCODE).o $(OBJS) -o $@

$(LLVMCODE).o: $(LLVMCODE).cpp $(LLVMCODE).h
	$(CLANG) $(LOGD) $(OPTD) -g $(LOGD) $(LDC) -c $(LLVMCODE).cpp

# llvm_file: test_llvm/$(TEST).c
# 	$(CLANG) -S -emit-llvm test_llvm/$(TEST).c -o test_llvm/$(TEST).ll

run:
	make all
	./$(source).out semantic_tests/$(TEST).c > $(TEST).ll
	clang main.c $(TEST).ll -o $(TEST).out
	./$(TEST).out

mem:
	make all
	$(MEM_CHECK) ./$(LLVMCODE) test_llvm/$(TEST).ll test_llvm/$(TEST)_out.ll

debug:
	make all
	$(DEBUGGER) ./$(LLVMCODE) test_llvm/$(TEST).ll test_llvm/$(TEST)_out.ll

clean:
	rm -rf lex.yy.c
	rm -rf y.tab.c y.tab.h
	rm -rf $(source).out y.output
	rm -rf $(source).out y.output
	rm -rf out*.c
	rm -rf *TRACE *.ll test_llvm/*.ll
	rm -rf $(LLVMCODE) *.o *.gch
