source = pset
LLVMCODE = optimizer
TEST = test1
OS := $(shell uname -s)
CLANG = clang++ --std=c++2a

ifeq ($(OS), Linux)
LDC=`llvm-config-15 --cflags` -I /usr/include/llvm-c-15/
CDC=`llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/
DEBUGGER=gdb --args
else
LDC=`llvm-config --cflags` -I /usr/include/llvm-c/
CDC=`llvm-config --cxxflags --ldflags --libs core` -I /usr/include/llvm-c/
DEBUGGER=lldb --
endif

ifeq ($(LOG), LOG)
LOGD=-DLOG
else
LOGD=
endif



all: $(LLVMCODE) llvm_file

.PHONY: all test valgrind debug

$(source).out: $(source).l $(source).y ast.h ast.c sem.h sem.c
	yacc -d -v -t $(source).y
	lex $(source).l
	g++ -g -o $(source).out lex.yy.c y.tab.c ast.c sem.c

valgrind:
	make clean
	make $(source).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p1.c &> out1.c
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p2.c &> out2.c
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p3.c &> out3.c
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p4.c &> out4.c

$(LLVMCODE): $(LLVMCODE).cpp $(LLVMCODE).h
	$(CLANG) -g $(LOGD) $(LDC) -c $(LLVMCODE).cpp
	$(CLANG) $(CDC) -I /usr/include/llvm-c/ $(LLVMCODE).o $(OBJS) -o $@

llvm_file: test_llvm/$(TEST).c
	$(CLANG) -S -emit-llvm test_llvm/$(TEST).c -o test_llvm/$(TEST).ll

run:
	make all
	./$(LLVMCODE) test_llvm/$(TEST).ll test_llvm/$(TEST)_out.ll

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
