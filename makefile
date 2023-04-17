source = pset

all: test

.PHONY: all test valgrind debug

$(source).out: $(source).l $(source).y
	yacc -d -v -t $(source).y
	lex $(source).l
	g++ -o $(source).out lex.yy.c y.tab.c ast.c

test:
	make clean
	make $(source).out
	./$(source).out test/p1.c 1> out1.c 2> TRACE1
	./$(source).out test/p2.c 1> out2.c 2> TRACE2
	./$(source).out test/p3.c 1> out3.c 2> TRACE3
	./$(source).out test/p4.c 1> out4.c 2> TRACE4
	./$(source).out test/p5.c 1> out5.c 2> TRACE5
	./$(source).out test/p_bad.c 1> out_bad.c 2> TRACE_BAD

debug:
	make clean
	make $(source).out
	echo "r" | gdb --args ./$(source).out test/p1.c 1> out1.c 2> TRACE1
	echo "r" | gdb --args ./$(source).out test/p2.c 1> out2.c 2> TRACE2
	echo "r" | gdb --args ./$(source).out test/p3.c 1> out3.c 2> TRACE3
	echo "r" | gdb --args ./$(source).out test/p4.c 1> out4.c 2> TRACE4
	echo "r" | gdb --args ./$(source).out test/p5.c 1> out5.c 2> TRACE5
	echo "r" | gdb --args ./$(source).out test/p_bad.c 1> out_bad.c 2> TRACE_BAD

valgrind:
	make clean
	make $(source).out
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p1.c 1> out1.c 2> TRACE1
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p2.c 1> out2.c 2> TRACE2
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p3.c 1> out3.c 2> TRACE3
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p4.c 1> out4.c 2> TRACE4
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p5.c 1> out5.c 2> TRACE5
	valgrind --leak-check=full --show-leak-kinds=all ./$(source).out test/p_bad.c 1> out_bad.c 2> TRACE_BAD

clean:
	rm -rf lex.yy.c y.tab.c y.tab.h $(source).out y.output out*.c TRACE*
