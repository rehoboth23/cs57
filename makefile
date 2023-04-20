source = pset

all: $(source).out

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

clean:
	rm -rf lex.yy.c y.tab.c y.tab.h $(source).out y.output out*.c TRACE*
