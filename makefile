source = pset
LLVMCODE = llvm_optimizer
TEST = test1
OBJS = optimizer.o
OS := $(shell uname -s)

all: $(LLVMCODE) llvm_file

.PHONY: all test valgrind debug

$(source).out: $(source).l $(source).y ast.h ast.c sem.h sem.c
	yacc -d -v -t $(source).y
	lex $(source).l
	g++ -g -o $(source).out lex.yy.c y.tab.c ast.c sem.c

test:
	make clean
	make $(source).out
	./$(source).out semantic_tests/p1.c &> out1.c
	./$(source).out semantic_tests/p2.c &> out2.c
	./$(source).out semantic_tests/p3.c &> out3.c
	./$(source).out semantic_tests/p4.c &> out4.c

debug:
	make clean
	make $(source).out
	echo "r" | gdb --args ./$(source).out semantic_tests/p1.c &> out1.c
	echo "r" | gdb --args ./$(source).out semantic_tests/p2.c &> out2.c
	echo "r" | gdb --args ./$(source).out semantic_tests/p3.c &> out3.c
	echo "r" | gdb --args ./$(source).out semantic_tests/p4.c &> out4.c

valgrind:
	make clean
	make $(source).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p1.c &> out1.c
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p2.c &> out2.c
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p3.c &> out3.c
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out semantic_tests/p4.c &> out4.c

$(LLVMCODE): $(LLVMCODE).c $(OBJS)
	clang++ -g `llvm-config-15 --cflags` -I /usr/include/llvm-c-15/ -c $(LLVMCODE).c
	clang++ `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ $(LLVMCODE).o $(OBJS) -o $@

llvm_file: test_llvm/$(TEST).c
	clang++ -S -emit-llvm test_llvm/$(TEST).c -o test_llvm/$(TEST).ll

run:
	make all
	./$(LLVMCODE) test_llvm/$(TEST).ll test_llvm/$(TEST)_out.ll

ifeq ($(OS), Linux)
debug_ll:
	gdb --args ./$(LLVMCODE) test_llvm/$(TEST).ll test_llvm/$(TEST)_out.ll
else
debug_ll:
	lldb -- ./$(LLVMCODE) test_llvm/$(TEST).ll test_llvm/$(TEST)_out.ll
endif

optimizer.o:
	clang++ -g -D$(LOG) `llvm-config-15 --cflags` -I /usr/include/llvm-c-15/ -c optimizer.h optimizer.c

clean:
	rm -rf lex.yy.c
	rm -rf y.tab.c y.tab.h
	rm -rf $(source).out y.output
	rm -rf $(source).out y.output
	rm -rf out*.c
	rm -rf *TRACE *.ll test_llvm/*.ll
	rm -rf $(LLVMCODE) *.o *.gch
