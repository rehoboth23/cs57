source = pset

all: $(source).out

.PHONY: all test valgrind debug

$(source).out: $(source).l $(source).y ast.h ast.c scope.h scope.c
	yacc -d -v -t $(source).y
	lex $(source).l
	g++ -o $(source).out lex.yy.c y.tab.c ast.c scope.c

test:
	make clean
	make $(source).out
	./$(source).out semantic_tests/p1.c &> out1.c
	./$(source).out semantic_tests/p2.c &> out2.c
	./$(source).out semantic_tests/p3.c &> out3.c
	./$(source).out semantic_tests/p4.c &> out4.c
	# ./$(source).out test/p5.c &> out5.c
	# ./$(source).out test/p_bad.c &> out_bad.c

debug:
	make clean
	make $(source).out
	echo "r" | gdb --args ./$(source).out test/p1.c &> out1.c 2> TRACE1
	echo "r" | gdb --args ./$(source).out test/p2.c &> out2.c 2> TRACE2
	echo "r" | gdb --args ./$(source).out test/p3.c &> out3.c 2> TRACE3
	echo "r" | gdb --args ./$(source).out test/p4.c &> out4.c 2> TRACE4
	echo "r" | gdb --args ./$(source).out test/p5.c &> out5.c 2> TRACE5
	echo "r" | gdb --args ./$(source).out test/p_bad.c &> out_bad.c 2> TRACE_BAD

valgrind:
	make clean
	make $(source).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p1.c &> out1.c 2> TRACE1
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p2.c &> out2.c 2> TRACE2
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p3.c &> out3.c 2> TRACE3
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p4.c &> out4.c 2> TRACE4
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p5.c &> out5.c 2> TRACE5
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p_bad.c &> out_bad.c 2> TRACE_BAD

clean:
	rm -rf lex.yy.c y.tab.c y.tab.h $(source).out y.output out*.c TRACE*
