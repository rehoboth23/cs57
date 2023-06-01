source = pset
OBJS = optimizer.o ir_gen.o codegen.o
TEST = p1
OS := $(shell uname -s)
CLANG = clang++ --std=c++2a

ifeq ($(OS), Linux)
LDC=`llvm-config-15 --cflags` -I/usr/include/llvm-c-15/ -I./
CDC=`llvm-config-15 --cxxflags --ldflags --libs core` -I/usr/include/llvm-c-15/ -I./
DEBUGGER=gdb --args
MEM_CHECK=valgrind --leak-check=full --show-leak-kinds=all
else
ARMD=-DARMD
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


all: $(OBJS) $(source).out

.PHONY: all mem debug

$(source).out: $(source).l $(source).y ast.h ast.c sem.h sem.cpp $(OBJS)
	yacc -d -v -t $(source).y
	lex $(source).l
	$(CLANG) -g $(CDC) $(OBJS) -o $(source).out lex.yy.c y.tab.c ast.c sem.cpp

ir_gen.o: ir_gen.cpp ir_gen.h
	$(CLANG) $(LOGD) $(OPTD) -g $(LDC) -c ir_gen.cpp -o $@

optimizer.o: optimizer.cpp optimizer.h
	$(CLANG) $(LOGD) $(OPTD) -g $(LDC) -c optimizer.cpp -o $@

codegen.o: codegen.cpp codegen.h
	$(CLANG) $(ARMD) $(LOGD) $(OPTD) -g $(LDC) -c codegen.cpp -o $@

run:
	make all
	./$(source).out semantic_tests/$(TEST).c $(TEST)
	gcc -m64 -g main.c $(TEST).s -o $(TEST).out
	./$(TEST).out

mem:
	make all
	$(MEM_CHECK) ./$(source).out semantic_tests/$(TEST).c $(TEST).ll
	# $(MEM_CHECK) ./$(TEST).out

debug:
	make all
	$(DEBUGGER) ./$(source).out semantic_tests/$(TEST).c $(TEST).ll
	$(DEBUGGER) ./$(TEST).out

clean:
	rm -rf lex.yy.c
	rm -rf y.tab.c y.tab.h
	rm -rf $(source).out y.output
	rm -rf $(source).out y.output
	rm -rf out*.c
	rm -rf *.s
	rm -rf *TRACE *.ll
	rm -rf *.o *.gch *.out
